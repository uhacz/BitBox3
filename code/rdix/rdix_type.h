#pragma once

#include <foundation/type.h>
#include <foundation/debug.h>
#include <foundation/tag.h>
#include <rdi_backend/rdi_backend_type.h>
#include <foundation/serializer.h>
// --- 
struct RDIXShaderFile
{
    static constexpr uint32_t TAG = BX_UTIL_TAG32( 'S','H','D','F' );
	static constexpr uint32_t VERSION = BX_UTIL_MAKE_VERSION( 1, 0, 0 );

	struct Pass
	{
		uint32_t hashed_name                = 0;
		RDIHardwareStateDesc hw_state_desc  = {};
		RDIVertexLayout vertex_layout       = {};
        uint32_t offset_resource_descriptor = 0;
        uint32_t size_resource_descriptor   = 0;
        uint32_t offset_bytecode[RDIEPipeline::COUNT] = {};
        uint32_t size_bytecode  [RDIEPipeline::COUNT] = {};
		
	};

    uint32_t tag        = TAG;
	uint32_t version    = VERSION;
	uint32_t num_passes = 0;
	Pass passes[1];
};

// --- 
struct RDIXPipelineDesc
{
	RDIXShaderFile* shader_file	 = nullptr;
	const char* shader_pass_name = nullptr;
	RDIHardwareStateDesc* hw_state_desc_override = nullptr;
	RDIETopology::Enum topology = RDIETopology::TRIANGLES;

    RDIXPipelineDesc() = default;
    RDIXPipelineDesc( RDIXShaderFile* f, const char* pass_name )
        : shader_file( f ), shader_pass_name( pass_name )
    {}

	RDIXPipelineDesc& Shader( RDIXShaderFile* f, const char* pass_name )
	{
		shader_file = f;
		shader_pass_name = pass_name;
		return *this;
	}
	RDIXPipelineDesc& HardwareState( RDIHardwareStateDesc* hwdesc ) { hw_state_desc_override = hwdesc; return *this; }
	RDIXPipelineDesc& Topology( RDIETopology::Enum t ) { topology = t; return *this; }
};

// ---
struct RDIXResourceSlot
{
	enum EType : uint8_t
	{
		READ_ONLY,
		READ_WRITE,
		UNIFORM,
		SAMPLER,
		_COUNT_,
	};

	const char* name = nullptr;
	EType type = EType::_COUNT_;
	uint8_t slot = 0;
	uint16_t stage_mask = 0;

	RDIXResourceSlot() {}
	RDIXResourceSlot( const char* n, EType t, uint8_t sm, uint8_t sl, uint8_t cnt )
		: name( n ), type( t ), slot( sl ), stage_mask( sm ) {}

	RDIXResourceSlot( const char* n, EType t ) { name = n;  type = t; }
	RDIXResourceSlot& StageMask( uint8_t sm ) { stage_mask = sm; return *this; }
	RDIXResourceSlot& Slot( uint8_t sl ) { slot = sl; return *this; }
};

// --- 
struct RDIXResourceLayout
{
	const RDIXResourceSlot* slots = nullptr;
	uint32_t count = 0;

	RDIXResourceLayout() {}
	RDIXResourceLayout( const RDIXResourceSlot* b, uint32_t n )
		: slots( b ), count( n ) {}
};

// --- 
struct RDIXResourceBindingMemoryRequirments
{
	uint32_t descriptor_size = 0;
	uint32_t data_size = 0;
	uint32_t Total() const { return descriptor_size + data_size; }
};

// --- 
struct RDIXRenderTargetDesc
{
	uint16_t num_color_textures = 0;
	uint16_t mips               = 1;
	uint16_t width              = 0;
	uint16_t height             = 0;

	RDIFormat color_texture_formats[cRDI_MAX_RENDER_TARGETS] = {};
	RDIEType::Enum depth_texture_type = RDIEType::UNKNOWN;

	RDIXRenderTargetDesc( uint16_t w, uint16_t h, uint16_t m = 1 )
	{
		width = w; height = h; mips = m;
	}
	RDIXRenderTargetDesc& Texture( RDIFormat format )
	{
		SYS_ASSERT( num_color_textures < cRDI_MAX_RENDER_TARGETS );
		color_texture_formats[num_color_textures++] = format;
		return *this;
	}
    RDIXRenderTargetDesc& Texture( uint32_t index, RDIFormat format )
    {
        SYS_ASSERT( num_color_textures == 0 );
        SYS_ASSERT( index < cRDI_MAX_RENDER_TARGETS );
        color_texture_formats[index] = format;
        return *this;
    }
	RDIXRenderTargetDesc& Depth( RDIEType::Enum dataType )
	{ 
		depth_texture_type = dataType; return *this; 
	}
};

// ---
struct RDIXRenderSourceRange
{
    uint32_t topology = RDIETopology::TRIANGLES;
    uint32_t begin = 0;
	uint32_t count = 0;
    uint32_t base_vertex = 0;

    RDIXRenderSourceRange() {}
    RDIXRenderSourceRange( uint32_t b, uint32_t c, uint32_t bv = 0 )
        : topology( RDIETopology::TRIANGLES )
        , begin( b ), count( c ), base_vertex( bv ) {}
};

struct RDIXRenderSourceDesc
{
	uint32_t num_vertices = 0;
	uint32_t num_indices = 0;
	uint32_t num_draw_ranges = 0;
	RDIVertexLayout vertex_layout = {};

	RDIIndexBuffer shared_index_buffer = {};
	RDIEType::Enum index_type = RDIEType::UNKNOWN;
	const void* index_data = nullptr;

	const void* vertex_data[cRDI_MAX_VERTEX_BUFFERS] = {};

	const RDIXRenderSourceRange* draw_ranges = nullptr;

	RDIXRenderSourceDesc& Count( uint32_t nVertices, uint32_t nIndices = 0 )
	{
		num_vertices = nVertices;
		num_indices = nIndices;
		return *this;
	}

	RDIXRenderSourceDesc& VertexBuffer( RDIVertexBufferDesc vbDesc, const void* initialData )
	{
		SYS_ASSERT( vertex_layout.count < cRDI_MAX_VERTEX_BUFFERS );
		uint32_t index = vertex_layout.count++;
		vertex_layout.descs[index] = vbDesc;
		vertex_data[index] = initialData;
		return *this;
	}
	RDIXRenderSourceDesc& IndexBuffer( RDIEType::Enum dt, const void* initialData )
	{
		SYS_ASSERT( shared_index_buffer.id == 0 );
		index_type = dt;
		index_data = initialData;
		return *this;
	}
	RDIXRenderSourceDesc& SharedIndexBuffer( RDIIndexBuffer ibuffer )
	{
		SYS_ASSERT( index_type == RDIEType::UNKNOWN );
		SYS_ASSERT( index_data == nullptr );
		shared_index_buffer = ibuffer;
		return *this;
	}
};

struct BIT_ALIGNMENT_16 RDIXMeshFile
{
    static constexpr uint32_t VERSION = BX_UTIL_MAKE_VERSION( 1, 1, 0 );
    static constexpr uint32_t TAG = BX_UTIL_TAG32( 'M','E','S','H' );

    uint32_t num_vertices = 0;
    uint32_t num_indices = 0;

    RDIVertexBufferDesc descs[RDIEVertexSlot::COUNT] = {};
    uint16_t num_streams = 0;
    uint16_t num_bones = 0;
    uint16_t num_draw_ranges = 0;
    uint16_t flag_use_16bit_indices = 0;

    uint32_t offset_streams[RDIEVertexSlot::COUNT] = {};
    uint32_t offset_indices = 0;
    uint32_t offset_bones = 0;
    uint32_t offset_bones_names = 0;
    uint32_t offset_draw_ranges = 0;   
};

template< typename T >
inline array_span_t<const T> GetVertexStream( const RDIXMeshFile* mesh, RDIEVertexSlot::Enum slot )
{
    RDIVertexBufferDesc desc = mesh->descs[slot];
    SYS_ASSERT( desc.ByteWidth() == sizeof( T ) );

    const T* pointer = Offset2Pointer<T>( mesh->offset_streams[slot] );
    return to_array_span( pointer, mesh->num_vertices );
}

template< typename T >
inline array_span_t<const T> GetIndexStream( const RDIXMeshFile* mesh )
{
    const T* pointer = Offset2Pointer<T>( mesh->offset_indices );
    return to_array_span<T>( pointer, mesh->num_indices );
}

inline array_span_t<const u16> GetIndexStream16( const RDIXMeshFile* mesh )
{
    SYS_ASSERT( mesh->flag_use_16bit_indices != 0 );
    return GetIndexStream<u16>( mesh );
}
inline array_span_t<const u32> GetIndexStream32( const RDIXMeshFile* mesh )
{
    SYS_ASSERT( mesh->flag_use_16bit_indices == 0 );
    return GetIndexStream<u32>( mesh );
}

// --- 
struct RDIXTransformBufferDesc
{
	uint32_t capacity = 1024;
};


struct RDIXPipeline;
struct RDIXResourceBinding;
struct RDIXRenderSource;
struct RDIXRenderTarget;
struct RDIXTransformBuffer;
