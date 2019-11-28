#include "rdix.h"

#include "rdix_command_buffer.h"

#include <filesystem/filesystem.h>
#include <memory/memory.h>
#include <foundation/math/vmath.h>
#include <foundation/hash.h>
#include <foundation/common.h>
#include <foundation/buffer.h>


#include <rdi_backend/rdi_backend.h>

#include <algorithm>

// --- Shader
RDIXShaderFile* LoadShaderFile( const char* name, BXIAllocator* allocator )
{
	BXFileWaitResult load_result = LoadFileSync( name, BXEFIleMode::BIN, allocator );
	
	RDIXShaderFile* sfile = CreateShaderFile( load_result.file.pointer, load_result.file.size );

	FileSys()->CloseFile( &load_result.handle, false );

	return sfile;
}

RDIXShaderFile* CreateShaderFile( const void * data, uint32_t dataSize )
{
	RDIXShaderFile* sfile = (RDIXShaderFile*)data;
	if( sfile->tag != RDIXShaderFile::TAG )
	{
		SYS_LOG_ERROR( "ShaderFile: wrong file tag!" );
		return nullptr;
	}

	if( sfile->version != RDIXShaderFile::VERSION )
	{
		SYS_LOG_ERROR( "ShaderFile: wrong version!" );
		return nullptr;
	}
		
	return sfile;
}

void UnloadShaderFile( RDIXShaderFile** shaderfile, BXIAllocator* allocator )
{
	BX_FREE0( allocator, shaderfile[0] );
}

uint32_t GenerateShaderFileHashedName( const char * name, uint32_t version )
{
	return murmur3_hash32( name, (uint32_t)strlen( name ), version );
}

uint32_t FindShaderPass( RDIXShaderFile* shaderfile, const char* passname )
{
	const uint32_t hashed_name = GenerateShaderFileHashedName( passname, shaderfile->version );
	for( uint32_t i = 0; i < shaderfile->num_passes; ++i )
	{
		if( hashed_name == shaderfile->passes[i].hashed_name )
			return i;
	}
	return UINT32_MAX;
}


//--- Pipeline
RDIXPipeline* CreatePipeline( RDIDevice* dev, const RDIXPipelineDesc& desc, BXIAllocator* allocator )
{
	RDIXPipeline* impl = (RDIXPipeline*)BX_NEW( allocator, RDIXPipeline );

	const uint32_t pass_index = FindShaderPass( desc.shader_file, desc.shader_pass_name );
	SYS_ASSERT( pass_index != UINT32_MAX );

	const RDIXShaderFile::Pass& pass = desc.shader_file->passes[pass_index];
	RDIShaderPassCreateInfo pass_create_info = {};
    for( uint32_t istage = 0; istage < RDIEPipeline::COUNT; ++istage )
    {
        pass_create_info.bytecode[istage] = TYPE_OFFSET_GET_POINTER( void, desc.shader_file->passes[pass_index].offset_bytecode[istage] );
        pass_create_info.bytecode_size[istage] = pass.size_bytecode[istage];
    }
 //   pass_create_info.vertex_bytecode = TYPE_OFFSET_GET_POINTER( void, desc.shader_file->passes[pass_index].offset_bytecode[RDIEPipeline::VERTEX] );
	//pass_create_info.vertex_bytecode_size = pass.size_bytecode[RDIEPipeline::VERTEX];
	//pass_create_info.pixel_bytecode = TYPE_OFFSET_GET_POINTER( void, desc.shader_file->passes[pass_index].offset_bytecode[RDIEPipeline::PIXEL] );
	//pass_create_info.pixel_bytecode_size = pass.size_bytecode[RDIEPipeline::PIXEL];
	pass_create_info.reflection = nullptr;
	impl->pass = CreateShaderPass( dev, pass_create_info );
	impl->pass.vertex_input_mask = pass.vertex_layout.InputMask();

    if( impl->pass.vertex_input_mask )
    {
        impl->input_layout = CreateInputLayout( dev, pass.vertex_layout, impl->pass );
    }
    
    if( impl->pass.vertex || impl->pass.pixel )
    {
        const RDIHardwareStateDesc* hw_state_desc = (desc.hw_state_desc_override) ? desc.hw_state_desc_override : &pass.hw_state_desc;
        impl->hardware_state = CreateHardwareState( dev, *hw_state_desc );
    }

	if( pass.offset_resource_descriptor )
	{
		void* src_resource_desc_ptr = TYPE_OFFSET_GET_POINTER( void, pass.offset_resource_descriptor );
		void* resource_desc_memory = BX_MALLOC( allocator, pass.size_resource_descriptor, 16 );
		memcpy( resource_desc_memory, src_resource_desc_ptr, pass.size_resource_descriptor );
		impl->resources = (RDIXResourceBinding*)resource_desc_memory;
	}

    impl->allocator = allocator;
	return impl;
}
void DestroyPipeline( RDIXPipeline** pipeline )
{
	if( !pipeline[0] )
		return;

    BXIAllocator* allocator = pipeline[0]->allocator;

	RDIXPipeline* pipe = pipeline[0];
	BX_FREE0( allocator, pipe->resources );
	Destroy( &pipe->hardware_state );
	Destroy( &pipe->input_layout );
	Destroy( &pipe->pass );
        
	BX_DELETE0( allocator, pipeline[0] );
}
void BindPipeline( RDICommandQueue* cmdq, RDIXPipeline* pipeline, const RDIXPipelineFlags& flags )
{
    if( flags.shaders )
    {
        SetShaderPass( cmdq, pipeline->pass );
        SetInputLayout( cmdq, pipeline->input_layout );
    }

    if( flags.hw_state )
    {
        SetHardwareState( cmdq, pipeline->hardware_state );
    }

	if( pipeline->resources && flags.resources )
	{
		BindResources( cmdq, pipeline->resources );
	}

	SetTopology( cmdq, pipeline->topology );
}
RDIXResourceBinding* ResourceBinding( const RDIXPipeline* p )
{
	return p->resources;
}

RDIXResourceBinding * CloneResourceBinding( const RDIXPipeline * p )
{
	return nullptr;
}



//---

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
struct RDIXResourceBinding
{
	struct Binding
	{
		uint8_t binding_type = RDIXResourceSlot::_COUNT_;
		uint8_t slot = 0;
		uint16_t stage_mask = 0;
		uint32_t data_offset = 0;
	};

	const uint32_t* HashedNames() const { return (uint32_t*)(this + 1); }
	const Binding* Bindings() const { return (Binding*)(HashedNames() + count); }
	uint8_t* Data() { return (uint8_t*)(Bindings() + count); }
	
    BXIAllocator* allocator = nullptr;
    RDIXResourceBindingMemoryRequirments mem;
	uint32_t count = 0;
};

namespace
{
	static const uint32_t _resource_size[RDIXResourceSlot::_COUNT_] =
	{
		sizeof( RDIResourceRO ),     //eRESOURCE_TYPE_READ_ONLY,
		sizeof( RDIResourceRW ),     //eRESOURCE_TYPE_READ_WRITE,
		sizeof( RDIConstantBuffer ), //eRESOURCE_TYPE_UNIFORM,
		sizeof( RDISampler ),        //eRESOURCE_TYPE_SAMPLER,
	};

	static uint32_t CalculateResourceBindingDescriprorSize( uint32_t nbindings )
	{
		uint32_t result = 0;
		result += sizeof( RDIXResourceBinding );
		result += nbindings * sizeof( uint32_t ); // hashed names
		result += nbindings * sizeof( RDIXResourceBinding::Binding );
		return result;
	}

}///

RDIXResourceBindingMemoryRequirments CalculateResourceBindingMemoryRequirments( const RDIXResourceLayout & layout )
{
	RDIXResourceBindingMemoryRequirments requirments = {};

	requirments.descriptor_size = CalculateResourceBindingDescriprorSize( layout.count );

	for( uint32_t i = 0; i < layout.count; ++i )
	{
		const RDIXResourceSlot slot = layout.slots[i];
		requirments.data_size += _resource_size[slot.type];
	}
	return requirments;
}

RDIXResourceBinding* CreateResourceBinding( const RDIXResourceLayout& layout, BXIAllocator* allocator )
{
	RDIXResourceBindingMemoryRequirments mem_req = CalculateResourceBindingMemoryRequirments( layout );

	const uint32_t mem_size = mem_req.Total();
	uint8_t* mem = (uint8_t*)BX_MALLOC( allocator, mem_size, 8 );
	memset( mem, 0x00, mem_size );

	RDIXResourceBinding* impl = (RDIXResourceBinding*)mem;
	impl->count = layout.count;
	impl->mem = mem_req;

	uint32_t* hashed_names = (uint32_t*)impl->HashedNames();
	RDIXResourceBinding::Binding* bindings = (RDIXResourceBinding::Binding*)impl->Bindings();

	SYS_ASSERT( (uintptr_t)(bindings + impl->count) == (uintptr_t)((uint8_t*)mem + mem_req.descriptor_size) );

	for( uint32_t i = 0; i < impl->count; ++i )
	{
		const RDIXResourceSlot& src_bind = layout.slots[i];
		RDIXResourceBinding::Binding& dst_bind = bindings[i];
		dst_bind.binding_type = src_bind.type;
		dst_bind.slot = src_bind.slot;
		dst_bind.stage_mask = src_bind.stage_mask;
		dst_bind.data_offset = GenerateResourceHashedName( src_bind.name );;
	}

	struct BindingCmp
	{
		inline bool operator()( RDIXResourceBinding::Binding a, RDIXResourceBinding::Binding b )
		{
			const uint32_t keya = uint32_t( a.binding_type ) << 24 | uint32_t( a.slot ) << 16 | a.stage_mask;
			const uint32_t keyb = uint32_t( b.binding_type ) << 24 | uint32_t( b.slot ) << 16 | b.stage_mask;
			return keya < keyb;
		}
	};
	std::sort( bindings, bindings + impl->count, BindingCmp() );

	uint32_t data_offset = 0;
	for( uint32_t i = 0; i < impl->count; ++i )
	{
		hashed_names[i] = bindings[i].data_offset;
		bindings[i].data_offset = data_offset;
		data_offset += _resource_size[bindings[i].binding_type];
	}

	SYS_ASSERT( data_offset == mem_req.data_size );
	SYS_ASSERT( (uintptr_t)(impl->Data() + data_offset) == (uintptr_t)((uint8_t*)mem + mem_size) );

#ifdef _DEBUG
	for( uint32_t i = 0; i < impl->count; ++i )
	{
		for( uint32_t j = i + 1; j < impl->count; ++j )
		{
			SYS_ASSERT( hashed_names[i] != hashed_names[j] );
		}
	}
#endif
    impl->allocator = allocator;
	return impl;
}

void DestroyResourceBinding( RDIXResourceBinding** binding )
{
    if( !binding[0] )
        return;

    BXIAllocator* allocator = binding[0]->allocator;
	BX_FREE0( allocator, binding[0] );
}

u32 GetMemorySize( const RDIXResourceBinding* binding )
{
    return (binding) ? binding->mem.Total() : 0;
}
void CloneResourceBinding( void* dst, u32 dst_size, const RDIXResourceBinding* binding )
{
    SYS_ASSERT( dst_size == binding->mem.Total() );
    RDIXResourceBinding* cloned = (RDIXResourceBinding*)dst;
    memcpy( cloned, binding, dst_size );
    cloned->allocator = nullptr;
}

RDIXResourceBinding* CloneResourceBinding( const RDIXResourceBinding* binding, BXIAllocator* allocator )
{
	const uint32_t mem_size = binding->mem.Total();
	RDIXResourceBinding* cloned = (RDIXResourceBinding*)BX_MALLOC( allocator, mem_size, 8 );
	memcpy( cloned, binding, mem_size );
    cloned->allocator = allocator;
	return cloned;
}

namespace
{
	static inline uint32_t _FindBinding( RDIXResourceBinding* binding, const char* name )
	{
		const uint32_t hashed_name = GenerateResourceHashedName( name );
		const uint32_t* resource_hashed_names = binding->HashedNames();
		for( uint32_t i = 0; i < binding->count; ++i )
		{
			if( hashed_name == resource_hashed_names[i] )
				return i;
		}
		return UINT32_MAX;
	}

	template< RDIXResourceSlot::EType B, typename T >
	void _SetResource1( RDIXResourceBinding* impl, uint32_t index, const T* resourcePtr )
	{
		SYS_ASSERT( index < impl->count );
		const RDIXResourceBinding::Binding& binding = impl->Bindings()[index];
		SYS_ASSERT( binding.binding_type == B );

		T* resource_data = (T*)(impl->Data() + binding.data_offset);
        resource_data[0] = (resourcePtr) ? *resourcePtr : T();
	}
	template< RDIXResourceSlot::EType B, typename T >
	bool _SetResource2( RDIXResourceBinding* impl, const char* name, const T* resourcePtr )
	{
		uint32_t index = _FindBinding( impl, name );
		if( index == UINT32_MAX )
		{
			SYS_LOG_ERROR( "Resource '%s' not found in descriptor", name );
			return false;
		}

		_SetResource1<B>( impl, index, resourcePtr );
		return true;
	}
}


bool ClearResource( RDICommandQueue * cmdq, RDIXResourceBinding* binding, const char * name )
{
	uint32_t index = _FindBinding( binding, name );
	if( index == UINT32_MAX )
	{
		SYS_LOG_ERROR( "Resource '%s' not found in descriptor", name );
		return false;
	}

	const RDIXResourceBinding::Binding& b = binding->Bindings()[index];
	switch( b.binding_type )
	{
	case RDIXResourceSlot::READ_ONLY:
	{
		RDIResourceRO null_resource = {};
		SetResourcesRO( cmdq, &null_resource, b.slot, 1, b.stage_mask );
	}break;
	case RDIXResourceSlot::READ_WRITE:
	{
		RDIResourceRW null_resource = {};
		SetResourcesRW( cmdq, &null_resource, b.slot, 1, b.stage_mask );
	}break;
	case RDIXResourceSlot::UNIFORM:
	{
		RDIConstantBuffer null_resource = {};
		SetCbuffers( cmdq, &null_resource, b.slot, 1, b.stage_mask );
	}break;
	case RDIXResourceSlot::SAMPLER:
	{
		RDISampler null_resource = {};
		SetSamplers( cmdq, &null_resource, b.slot, 1, b.stage_mask );
	}break;
	default:
		SYS_NOT_IMPLEMENTED;
		break;
	}
	return true;
}

void BindResources( RDICommandQueue* cmdq, RDIXResourceBinding* rbind )
{
	const RDIXResourceBinding::Binding* bindings = rbind->Bindings();
	uint8_t* data = rbind->Data();
	const uint32_t n = rbind->count;

	for( uint32_t i = 0; i < n; ++i )
	{
		const RDIXResourceBinding::Binding b = bindings[i];
		uint8_t* resource_data = data + b.data_offset;
		switch( b.binding_type )
		{
		case RDIXResourceSlot::READ_ONLY:
		{
			RDIResourceRO* r = (RDIResourceRO*)resource_data;
			SetResourcesRO( cmdq, r, b.slot, 1, b.stage_mask );
		}break;
		case RDIXResourceSlot::READ_WRITE:
		{
			RDIResourceRW* r = (RDIResourceRW*)resource_data;
			SetResourcesRW( cmdq, r, b.slot, 1, b.stage_mask );
		}break;
		case RDIXResourceSlot::UNIFORM:
		{
			RDIConstantBuffer* r = (RDIConstantBuffer*)resource_data;
			SetCbuffers( cmdq, r, b.slot, 1, b.stage_mask );
		}break;
		case RDIXResourceSlot::SAMPLER:
		{
			RDISampler* r = (RDISampler*)resource_data;
			SetSamplers( cmdq, r, b.slot, 1, b.stage_mask );
		}break;
		default:
			SYS_NOT_IMPLEMENTED;
			break;
		}
	}
}

uint32_t FindResource( RDIXResourceBinding* binding, const char* name )
{
	return _FindBinding( binding, name );
}

void SetResourceROByIndex    ( RDIXResourceBinding* rbind, uint32_t index, const RDIResourceRO* resource )     { _SetResource1<RDIXResourceSlot::READ_ONLY>( rbind, index, resource );}
void SetResourceRWByIndex    ( RDIXResourceBinding* rbind, uint32_t index, const RDIResourceRW* resource )     { _SetResource1<RDIXResourceSlot::READ_WRITE>( rbind, index, resource );}
void SetConstantBufferByIndex( RDIXResourceBinding* rbind, uint32_t index, const RDIConstantBuffer* cbuffer )  { _SetResource1<RDIXResourceSlot::UNIFORM>( rbind, index, cbuffer );}
void SetSamplerByIndex       ( RDIXResourceBinding* rbind, uint32_t index, const RDISampler* sampler )         { _SetResource1<RDIXResourceSlot::SAMPLER>( rbind, index, sampler );}
bool SetResourceRO           ( RDIXResourceBinding* rbind, const char* name, const RDIResourceRO* resource )   { return _SetResource2<RDIXResourceSlot::READ_ONLY>( rbind, name, resource );}
bool SetResourceRW           ( RDIXResourceBinding* rbind, const char* name, const RDIResourceRW* resource )   { return _SetResource2<RDIXResourceSlot::READ_WRITE>( rbind, name, resource );}
bool SetConstantBuffer       ( RDIXResourceBinding* rbind, const char* name, const RDIConstantBuffer* cbuffer ){ return _SetResource2<RDIXResourceSlot::UNIFORM>( rbind, name, cbuffer );}
bool SetSampler              ( RDIXResourceBinding* rbind, const char* name, const RDISampler* sampler )       { return _SetResource2<RDIXResourceSlot::SAMPLER>( rbind, name, sampler );}

uint32_t GenerateResourceHashedName( const char * name )
{
	return murmur3_hash32( name, (uint32_t)strlen( name ), tag32_t( "RDES" ) );
}






//---
struct RDIXRenderTarget
{
	RDITextureRW color_textures[cRDI_MAX_RENDER_TARGETS] = {};
	RDITextureDepth depth_texture;
	uint32_t num_color_textures = 0;
    BXIAllocator* allocator = nullptr;
};
RDIXRenderTarget* CreateRenderTarget( RDIDevice* dev, const RDIXRenderTargetDesc& desc, BXIAllocator* allocator )
{
    RDIXRenderTarget* impl = BX_NEW( allocator, RDIXRenderTarget );

    uint32_t num_color_textures = 0;
    if( !desc.num_color_textures )
    {
        for( ; num_color_textures < cRDI_MAX_RENDER_TARGETS; ++num_color_textures )
        {
            if( desc.color_texture_formats[num_color_textures].type == RDIEType::UNKNOWN )
                break;
        }
    }
    else
    {
        num_color_textures = desc.num_color_textures;
    }
    
    for( uint32_t i = 0; i < num_color_textures; ++i )
	{
		impl->color_textures[i] = CreateTexture2D( dev, desc.width, desc.height, desc.mips, desc.color_texture_formats[i], RDIEBind::RENDER_TARGET | RDIEBind::SHADER_RESOURCE, 0, nullptr );
	}
	impl->num_color_textures = num_color_textures;

	if( desc.depth_texture_type != RDIEType::UNKNOWN )
	{
		impl->depth_texture = CreateTexture2Ddepth( dev, desc.width, desc.height, desc.mips, desc.depth_texture_type );
	}
    impl->allocator = allocator;
	return impl;
}
void DestroyRenderTarget( RDIDevice* dev, RDIXRenderTarget** renderTarget )
{
	if( renderTarget[0] == nullptr )
		return;

	RDIXRenderTarget* impl = renderTarget[0];

	if( impl->depth_texture.id )
	{
		Destroy( &impl->depth_texture );
	}

	for( uint32_t i = 0; i < impl->num_color_textures; ++i )
	{
		Destroy( &impl->color_textures[i] );
	}

    BXIAllocator* allocator = impl->allocator;
	BX_DELETE0( allocator, renderTarget[0] );
}

void ClearRenderTarget( RDICommandQueue * cmdq, RDIXRenderTarget * rtarget, float rgbad[5] )
{
	int clear_depth = (rgbad[4] >= 0.f) ? 1 : 0;
	ClearBuffers( cmdq, rtarget->color_textures, rtarget->num_color_textures, rtarget->depth_texture, rgbad, 1, clear_depth );
}

void ClearRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* rtarget, float r, float g, float b, float a, float d )
{
	float rgbad[5] = { r, g, b, a, d };
	int clear_depth = (d >= 0.f) ? 1 : 0;
	ClearBuffers( cmdq, rtarget->color_textures, rtarget->num_color_textures, rtarget->depth_texture, rgbad, 1, clear_depth );
}

void ClearRenderTargetDepth( RDICommandQueue* cmdq, RDIXRenderTarget* rtarget, float d )
{
	float rgbad[5] = { 0.f, 0.f, 0.f, 0.f, d };
	ClearBuffers( cmdq, nullptr, 0, rtarget->depth_texture, rgbad, 0, 1 );
}

void BindRenderTarget( RDICommandQueue * cmdq, RDIXRenderTarget * renderTarget, const std::initializer_list<uint8_t>& colorTextureIndices, bool useDepth )
{
	SYS_ASSERT( (uint32_t)colorTextureIndices.size() < cRDI_MAX_RENDER_TARGETS );

	RDITextureRW color[cRDI_MAX_RENDER_TARGETS] = {};
	uint8_t color_array_index = 0;
	for( uint8_t i : colorTextureIndices )
	{
		SYS_ASSERT( i < renderTarget->num_color_textures );
		color[color_array_index++] = renderTarget->color_textures[i];
	}

	RDITextureDepth depth = {};
	if( useDepth )
	{
		depth = renderTarget->depth_texture;
	}

	ChangeRenderTargets( cmdq, color, (uint32_t)colorTextureIndices.size(), depth );
}

void BindRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* renderTarget, uint8_t color_texture_mask, bool use_depth )
{
    RDITextureRW color[cRDI_MAX_RENDER_TARGETS] = {};

    uint32_t num_color_textures = 0;
    for( uint8_t i = 0; color_texture_mask; ++i )
    {
        if( num_color_textures >= renderTarget->num_color_textures )
            break;

        if( color_texture_mask & 0x1 )
            color[num_color_textures++] = renderTarget->color_textures[i];
        
        color_texture_mask = color_texture_mask >> 1;
    }

    RDITextureDepth depth = {};
    if( use_depth )
    {
        depth = renderTarget->depth_texture;
    }

    ChangeRenderTargets( cmdq, color, num_color_textures, depth );
}

void BindRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* renderTarget )
{
	ChangeRenderTargets( cmdq, renderTarget->color_textures, renderTarget->num_color_textures, renderTarget->depth_texture );
}


RDITextureRW Texture( RDIXRenderTarget * rtarget, uint32_t index )
{
	SYS_ASSERT( index < rtarget->num_color_textures );
	return rtarget->color_textures[index];
}

RDITextureDepth TextureDepth( RDIXRenderTarget * rtarget )
{
	return rtarget->depth_texture;
}


// --- RenderSource
struct RDIXRenderSource
{
	uint8_t num_vertex_buffers = 0;
	uint8_t num_draw_ranges = 0;
    uint16_t managed_buffers_mask = 0;
    
    RDIIndexBuffer index_buffer;
	RDIVertexBuffer* vertex_buffers = nullptr;
	RDIXRenderSourceRange* draw_ranges = nullptr;

    BXIAllocator* allocator = nullptr;

    static constexpr uint16_t MANAGED_INDEX_BUFFER_MASK = BIT_OFFSET( 15 );

    bool IsIndexBufferManaged() const { return (managed_buffers_mask & MANAGED_INDEX_BUFFER_MASK) != 0; }
    bool IsBufferManaged( uint32_t index ) const { return (managed_buffers_mask & BIT_OFFSET( index )) != 0; }
};
RDIXRenderSource* CreateRenderSource( RDIDevice* dev, const RDIXRenderSourceDesc& desc, BXIAllocator* allocator )
{
	const uint32_t num_streams = desc.vertex_layout.count;
    const uint32_t num_draw_ranges = desc.num_draw_ranges + 1; // max_of_2( 1u, desc.num_draw_ranges );

	uint32_t mem_size = sizeof( RDIXRenderSource );
	mem_size += (num_streams) * sizeof( RDIVertexBuffer );
	mem_size += num_draw_ranges * sizeof( RDIXRenderSourceRange );

	void* mem = BX_MALLOC( allocator, mem_size, ALIGNOF( RDIXRenderSource ) );
	memset( mem, 0x00, mem_size );

	BufferChunker chunker( mem, mem_size );
	RDIXRenderSource* impl = chunker.Add< RDIXRenderSource >();
	impl->vertex_buffers   = chunker.Add< RDIVertexBuffer >( num_streams );
	impl->draw_ranges      = chunker.Add< RDIXRenderSourceRange >( num_draw_ranges );
	chunker.Check();

	impl->num_vertex_buffers = num_streams;
	impl->num_draw_ranges = num_draw_ranges;

	for( uint32_t i = 0; i < num_streams; ++i )
	{
		const void* data = desc.vertex_data[i];
		impl->vertex_buffers[i] = CreateVertexBuffer( dev, desc.vertex_layout.descs[i], desc.num_vertices, data );
        impl->managed_buffers_mask |= BIT_OFFSET( i );
	}

	RDIXRenderSourceRange& default_range = impl->draw_ranges[impl->num_draw_ranges-1];
	if( desc.index_type != RDIEType::UNKNOWN )
	{
		impl->index_buffer = CreateIndexBuffer( dev, desc.index_type, desc.num_indices, desc.index_data );
		default_range.count = desc.num_indices;
        impl->managed_buffers_mask |= RDIXRenderSource::MANAGED_INDEX_BUFFER_MASK;
	}
	else if( desc.shared_index_buffer.id )
	{
		impl->index_buffer = desc.shared_index_buffer;
		default_range.count = desc.num_indices;
	}
	else
	{
		default_range.count = desc.num_vertices;
	}

	for( uint32_t i = 0; i < desc.num_draw_ranges; ++i )
	{
		impl->draw_ranges[i] = desc.draw_ranges[i];
	}

    impl->allocator = allocator;

	return impl;
}

static RDIXRenderSource* CloneForSkinning( RDIDevice* dev, const RDIXRenderSource* base, uint32_t skinned_slot_mask, bool gpu_mode )
{
    constexpr uint32_t ignore_slot_mask = BIT_OFFSET( RDIEVertexSlot::BLENDINDICES ) | BIT_OFFSET( RDIEVertexSlot::BLENDWEIGHT );

    BXIAllocator* allocator = base->allocator;
    const uint32_t num_streams = base->num_vertex_buffers;
    const uint32_t num_draw_ranges = base->num_draw_ranges;

    uint32_t mem_size = sizeof( RDIXRenderSource );
    mem_size += num_streams * sizeof( RDIVertexBuffer );
    mem_size += num_draw_ranges * sizeof( RDIXRenderSourceRange );

    void* mem = BX_MALLOC( allocator, mem_size, ALIGNOF( RDIXRenderSource ) );
    memset( mem, 0x00, mem_size );

    BufferChunker chunker( mem, mem_size );
    RDIXRenderSource* impl = chunker.Add< RDIXRenderSource >();
    impl->vertex_buffers = chunker.Add< RDIVertexBuffer >( num_streams );
    impl->draw_ranges = chunker.Add< RDIXRenderSourceRange >( num_draw_ranges );
    chunker.Check();

    impl->num_vertex_buffers = num_streams;
    impl->num_draw_ranges = num_draw_ranges;

    for( uint32_t i = 0; i < num_streams; ++i )
    {
        const RDIVertexBuffer vbuffer = base->vertex_buffers[i];
        RDIVertexBufferDesc desc = vbuffer.desc;
        const uint32_t current_slot_mask = BIT_OFFSET( desc.slot );
        if( current_slot_mask & skinned_slot_mask )
        {
            if( gpu_mode )
            {
                desc.GPUWrite();
            }
            else
            {
                desc.CPUWrite();
            }

            impl->vertex_buffers[i] = CreateVertexBuffer( dev, desc, vbuffer.numElements, nullptr );
            impl->managed_buffers_mask |= BIT_OFFSET( i );
        }
        else if( (current_slot_mask & ignore_slot_mask) == 0 )
        {
            impl->vertex_buffers[i] = vbuffer;
        }
    }

    for( uint32_t i = 0; i < num_draw_ranges; ++i )
    {
        impl->draw_ranges[i] = base->draw_ranges[i];
    }

    impl->index_buffer = base->index_buffer;
    impl->allocator = allocator;

    return impl;
}

RDIXRenderSource* CloneForCPUSkinning( RDIDevice* dev, const RDIXRenderSource* base, uint32_t skinned_slot_mask )
{
    return CloneForSkinning( dev, base, skinned_slot_mask, false );
}

RDIXRenderSource* CloneForGPUSkinning( RDIDevice* dev, const RDIXRenderSource* base, uint32_t skinned_slot_mask /*= RDIEVertexSlot::SkinningMaskPosNrm() */ )
{
    return CloneForSkinning( dev, base, skinned_slot_mask, true );
}

RDIXRenderSource * CreateRenderSourceFromShape( RDIDevice* dev, const par_shapes_mesh* shape, BXIAllocator* allocator )
{
	RDIXRenderSourceDesc desc = {};
	desc.Count( shape->npoints, shape->ntriangles * 3 );
	desc.VertexBuffer( RDIVertexBufferDesc::POS(), shape->points );
	desc.VertexBuffer( RDIVertexBufferDesc::NRM(), shape->normals );
	desc.VertexBuffer( RDIVertexBufferDesc::UV0(), shape->tcoords );
	desc.IndexBuffer( RDIEType::USHORT, shape->triangles );

	return CreateRenderSource( dev, desc, allocator );
}

RDIXRenderSource* CreateRenderSourceFromShape( RDIDevice* dev, const poly_shape_t* shape, BXIAllocator* allocator )
{
    RDIXRenderSourceDesc desc = {};
    desc.Count( shape->num_vertices, shape->num_indices );
    desc.VertexBuffer( RDIVertexBufferDesc::POS(), shape->positions );
    desc.VertexBuffer( RDIVertexBufferDesc::NRM(), shape->normals );
    desc.VertexBuffer( RDIVertexBufferDesc::UV0(), shape->texcoords );
    desc.IndexBuffer( RDIEType::UINT, shape->indices );

    return CreateRenderSource( dev, desc, allocator );
}

RDIXRenderSource* CreateRenderSourceFromMemory( RDIDevice* dev, const RDIXMeshFile* header, BXIAllocator* allocator )
{
    RDIXRenderSourceDesc desc = {};
    desc.Count( header->num_vertices, header->num_indices );

    const uint32_t num_streams = header->num_streams;
    for( uint32_t i = 0; i < num_streams; ++i )
    {
        const RDIVertexBufferDesc& stream_desc = header->descs[i];
        const void* data_pointer = TYPE_OFFSET_GET_POINTER( void, header->offset_streams[i] );

        desc.VertexBuffer( stream_desc, data_pointer );
    }

    if( header->num_indices )
    {
        const RDIEType::Enum type = (header->flag_use_16bit_indices) ? RDIEType::USHORT : RDIEType::UINT;
        const void* data_pointer = TYPE_OFFSET_GET_POINTER( void, header->offset_indices );

        desc.IndexBuffer( type, data_pointer );
    }

    return CreateRenderSource( dev, desc, allocator );
}

void DestroyRenderSource( RDIXRenderSource** rsource )
{
	if( !rsource[0] )
		return;

	RDIXRenderSource* impl = rsource[0];

	if( impl->IsIndexBufferManaged() )
	{
		Destroy( &impl->index_buffer );
	}
	for( uint32_t i = 0; i < impl->num_vertex_buffers; ++i )
	{
        if( impl->IsBufferManaged( i ) )
        {
            Destroy( &impl->vertex_buffers[i] );
        }
    }

    BXIAllocator* allocator = impl->allocator;
	BX_FREE0( allocator, rsource[0] );
}

void BindRenderSource( RDICommandQueue* cmdq, RDIXRenderSource* renderSource )
{
	RDIVertexBuffer* vbuffers = nullptr;
	uint32_t num_vbuffers = 0;

	RDIIndexBuffer ibuffer = {};

	if( renderSource )
	{
		vbuffers = renderSource->vertex_buffers;
		num_vbuffers = renderSource->num_vertex_buffers;
		ibuffer = renderSource->index_buffer;
	}
	SetVertexBuffers( cmdq, vbuffers, 0, num_vbuffers );
	SetIndexBuffer( cmdq, ibuffer );
}
void SubmitRenderSource( RDICommandQueue* cmdq, RDIXRenderSource* renderSource, uint32_t rangeIndex )
{
	SYS_ASSERT( rangeIndex < renderSource->num_draw_ranges );
	RDIXRenderSourceRange range = renderSource->draw_ranges[rangeIndex];
    SubmitRenderSource( cmdq, renderSource, range );	
}

void SubmitRenderSource( RDICommandQueue* cmdq, RDIXRenderSource* renderSource, const RDIXRenderSourceRange& range )
{
    SetTopology( cmdq, range.topology );
    if( renderSource->index_buffer.id )
        DrawIndexed( cmdq, range.count, range.begin, 0 );
    else
        Draw( cmdq, range.count, range.begin );
}

void SubmitRenderSourceInstanced( RDICommandQueue* cmdq, RDIXRenderSource* renderSource, uint32_t numInstances, uint32_t rangeIndex )
{
	SYS_ASSERT( rangeIndex < renderSource->num_draw_ranges );
	RDIXRenderSourceRange range = renderSource->draw_ranges[rangeIndex];

	if( renderSource->index_buffer.id )
		DrawIndexedInstanced( cmdq, range.count, range.begin, numInstances, range.base_vertex );
	else
		DrawInstanced( cmdq, range.count, range.begin, numInstances );
}

uint32_t NumVertexBuffers( const RDIXRenderSource* rsource ){ return rsource->num_vertex_buffers; }
uint32_t NumVertices     ( const RDIXRenderSource* rsource ){ return rsource->vertex_buffers[0].numElements; }
uint32_t NumIndices      ( const RDIXRenderSource* rsource ){ return rsource->index_buffer.numElements; }
uint32_t NumRanges       ( const RDIXRenderSource* rsource ){ return rsource->num_draw_ranges; }

RDIVertexBuffer FindVertexBuffer( const RDIXRenderSource* rsource, RDIEVertexSlot::Enum slot )
{
    for( uint32_t i = 0; i < rsource->num_vertex_buffers; ++i )
    {
        if( rsource->vertex_buffers[i].desc.slot == slot )
            return rsource->vertex_buffers[i];
    }

    return RDIVertexBuffer();
}

RDIVertexBuffer VertexBuffer( const RDIXRenderSource* rsource, uint32_t index )
{
	SYS_ASSERT( index < rsource->num_vertex_buffers );
	return rsource->vertex_buffers[index];
}
RDIIndexBuffer IndexBuffer( const RDIXRenderSource* rsource ){ return rsource->index_buffer; }
RDIXRenderSourceRange Range( const RDIXRenderSource* rsource, uint32_t index )
{
	SYS_ASSERT( index < rsource->num_draw_ranges );
	return rsource->draw_ranges[index];
}


// ---
namespace rdix
{
	struct Matrix
	{
		vec4_t row0;
		vec4_t row1;
		vec4_t row2;
	};
	struct MatrixIT
	{
		vec3_t row0;
		vec3_t row1;
		vec3_t row2;
	};
}//
struct BIT_ALIGNMENT_16 RDIXTransformBuffer
{
	static constexpr uint32_t NUM_ROWS_PER_MATRIX = 3;
	
	RDIConstantBuffer gpu_instance_offset = {};
	RDIBufferRO gpu_buffer_matrix = {};
	RDIBufferRO gpu_buffer_matrix_it = {};

	uint32_t max_elements = 0;
	uint32_t num_elements = 0;
		
	rdix::Matrix*   MatrixData()   { return (rdix::Matrix*)(this + 1); }
	rdix::MatrixIT* MatrixITData() { return (rdix::MatrixIT*)(MatrixData() + max_elements); }

	rdix::Matrix*   Matrix  ( uint32_t index ) { return MatrixData() + index; }
	rdix::MatrixIT* MatrixIT( uint32_t index ) { return MatrixITData() + index; }
};

RDIXTransformBuffer* CreateTransformBuffer( RDIDevice* dev, const RDIXTransformBufferDesc& desc, BXIAllocator* allocator )
{
	// m -> matrix
	// mit -> matrix inverse transpose
	uint32_t mem_size = sizeof( RDIXTransformBuffer );
	mem_size += desc.capacity * sizeof( rdix::Matrix );
	mem_size += desc.capacity * sizeof( rdix::MatrixIT );

	RDIXTransformBuffer* buffer = (RDIXTransformBuffer*)BX_MALLOC( allocator, mem_size, 16 );
	memset( buffer, 0x00, mem_size );

    const RDIFormat gpu_format_m = RDIFormat::Float4();
    const RDIFormat gpu_format_mit = RDIFormat::Float3();
    const uint32_t num_elements = 3 * desc.capacity; // 3 x float4 (matrix) or 3 x float3 (matrix it)

	buffer->gpu_instance_offset = CreateConstantBuffer( dev, 16 );
	buffer->gpu_buffer_matrix = CreateBufferRO( dev, num_elements, gpu_format_m, RDIECpuAccess::WRITE );
	buffer->gpu_buffer_matrix_it = CreateBufferRO( dev, num_elements, gpu_format_mit, RDIECpuAccess::WRITE );
	buffer->max_elements = desc.capacity;

	return buffer;
}

void DestroyTransformBuffer( RDIXTransformBuffer** buffer, BXIAllocator* allocator )
{
	if( !buffer[0] )
		return;

	Destroy( &buffer[0]->gpu_buffer_matrix_it );
	Destroy( &buffer[0]->gpu_buffer_matrix );
	Destroy( &buffer[0]->gpu_instance_offset );

	BX_FREE0( allocator, buffer[0] );
}

void ClearTransformBuffer( RDIXTransformBuffer* buffer )
{
	buffer->num_elements = 0;
}

uint32_t GetDataCapacity( RDIXTransformBuffer* buffer )
{
    return buffer->max_elements * sizeof( mat44_t );
}

uint32_t AppendMatrix( RDIXTransformBuffer* buffer, const mat44_t& matrix )
{
	if( buffer->num_elements >= buffer->max_elements )
		return false;

	const uint32_t index = buffer->num_elements++;
	mat44_t tmp = transpose( matrix );

	rdix::Matrix* dst = buffer->Matrix( index );
	dst->row0 = tmp.c0;
	dst->row1 = tmp.c1;
	dst->row2 = tmp.c2;

	tmp = inverse( matrix );
	rdix::MatrixIT* dst_it = buffer->MatrixIT( index );
	dst_it->row0 = tmp.c0.xyz();
	dst_it->row1 = tmp.c1.xyz();
	dst_it->row2 = tmp.c2.xyz();

	return index;
}

void UploadTransformBuffer( RDICommandQueue* cmdq, RDIXTransformBuffer* buffer )
{
	SYS_STATIC_ASSERT( sizeof( rdix::Matrix ) == 3 * sizeof( vec4_t ) );

	uint8_t* dst = nullptr;

	dst = Map( cmdq, buffer->gpu_buffer_matrix, 0, RDIEMapType::WRITE );
	memcpy( dst, buffer->MatrixData(), buffer->num_elements * sizeof( rdix::Matrix ) );
	Unmap( cmdq, buffer->gpu_buffer_matrix );

	dst = Map( cmdq, buffer->gpu_buffer_matrix_it, 0, RDIEMapType::WRITE );
	memcpy( dst, buffer->MatrixITData(), buffer->num_elements * sizeof( rdix::MatrixIT ) );
	Unmap( cmdq, buffer->gpu_buffer_matrix_it );
}

RDIXCommand* UploadTransformBuffer( RDIXCommandBuffer* cmdbuff, RDIXCommand* parentcmd, RDIXTransformBuffer* buffer )
{
	const uint32_t data_size1 = buffer->num_elements * sizeof( rdix::Matrix );
	const uint32_t data_size2 = buffer->num_elements * sizeof( rdix::MatrixIT );

	RDIXUpdateBufferCmd* cmd1 = AllocateCommandWithData<RDIXUpdateBufferCmd>( cmdbuff, data_size1, parentcmd );
	cmd1->resource = buffer->gpu_buffer_matrix;
	cmd1->size = data_size1;
	memcpy( cmd1->DataPtr(), buffer->MatrixData(), data_size1 );

	RDIXUpdateBufferCmd* cmd2 = AllocateCommandWithData<RDIXUpdateBufferCmd>( cmdbuff, data_size1, cmd1 );
	cmd2->resource = buffer->gpu_buffer_matrix_it;
	cmd2->size = data_size2;
	memcpy( cmd2->DataPtr(), buffer->MatrixITData(), data_size2 );

	return cmd2;
}
void BindTransformBuffer( RDICommandQueue* cmdq, RDIXTransformBuffer* buffer, const RDIXTransformBufferBindInfo& bind_info )
{
    SetCbuffers( cmdq, &buffer->gpu_instance_offset, bind_info.instance_offset_slot, 1, bind_info.stage_mask );
	SetResourcesRO( cmdq, &buffer->gpu_buffer_matrix, bind_info.matrix_start_slot, 2, bind_info.stage_mask );
}

RDIXCommand* BindTransformBuffer( RDIXCommandBuffer* cmdbuff, RDIXCommand* parentcmd, RDIXTransformBuffer* buffer, const RDIXTransformBufferBindInfo& bind_info )
{
 	auto* cmd1 = AllocateCommand<RDIXSetResourceROCmd>( cmdbuff, parentcmd );
	cmd1->resource = buffer->gpu_buffer_matrix;
	cmd1->slot = bind_info.matrix_start_slot;
	cmd1->stage_mask = bind_info.stage_mask;

	auto* cmd2 = AllocateCommand<RDIXSetResourceROCmd>( cmdbuff, cmd1 );
	cmd2->resource = buffer->gpu_buffer_matrix_it;
	cmd2->slot = bind_info.matrix_start_slot + 1;
	cmd2->stage_mask = bind_info.stage_mask;

    auto* cmd3 = AllocateCommand<RDIXSetConstantBufferCmd>( cmdbuff, cmd2 );
    cmd3->resource = buffer->gpu_instance_offset;
    cmd3->slot = bind_info.instance_offset_slot;
    cmd3->stage_mask = bind_info.stage_mask;

	return cmd3;
}

RDIXTransformBufferCommands UploadAndSetTransformBuffer( RDIXCommandBuffer * cmdbuff, RDIXCommand * parentcmd, RDIXTransformBuffer * buffer, const RDIXTransformBufferBindInfo& bind_info )
{	
	const uint32_t data_size1 = buffer->num_elements * sizeof( rdix::Matrix );
	const uint32_t data_size2 = buffer->num_elements * sizeof( rdix::MatrixIT );

    RDIXCommandChain chain( cmdbuff, parentcmd );
	RDIXUpdateBufferCmd* cmd1 = chain.AppendCmdWithData<RDIXUpdateBufferCmd>( data_size1, buffer->gpu_buffer_matrix, buffer->MatrixData(), data_size1 );
    RDIXUpdateBufferCmd* cmd2 = chain.AppendCmdWithData<RDIXUpdateBufferCmd>( data_size2, buffer->gpu_buffer_matrix_it, buffer->MatrixITData(), data_size2 );
    RDIXSetResourceROCmd* cmd3 = chain.AppendCmd<RDIXSetResourceROCmd>( buffer->gpu_buffer_matrix, bind_info.matrix_start_slot, bind_info.stage_mask );
    RDIXSetResourceROCmd* cmd4 = chain.AppendCmd<RDIXSetResourceROCmd>( buffer->gpu_buffer_matrix_it, bind_info.matrix_start_slot + 1, bind_info.stage_mask );
    RDIXSetConstantBufferCmd* cmd5 = chain.AppendCmd<RDIXSetConstantBufferCmd>( buffer->gpu_instance_offset, bind_info.instance_offset_slot, bind_info.stage_mask );
	return { cmd1, cmd5 };
}

RDIConstantBuffer GetInstanceOffsetCBuffer( RDIXTransformBuffer * cmdbuff )
{
    return cmdbuff->gpu_instance_offset;
}
