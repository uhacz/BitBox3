#include "rdi_backend_dx11.h"
#include "rdi_shader_reflection.h"
#include "DirectXTex/DirectXTex.h"

#include <memory/memory.h>
#include <foundation/hash.h>
#include <foundation/common.h>

#include <D3Dcompiler.h>

namespace bx { namespace rdi {

void StartupDX11( RDIDevice** dev, RDICommandQueue** cmdq, uintptr_t hWnd, int winWidth, int winHeight, int fullScreen, BXIAllocator* allocator )
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 2;
    sd.BufferDesc.Width = winWidth;
    sd.BufferDesc.Height = winHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = (HWND)hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = ( fullScreen ) ? FALSE : TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT flags = 0;//D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    IDXGISwapChain* dx11SwapChain = 0;
    ID3D11Device* dx11Dev = 0;
    ID3D11DeviceContext* dx11Ctx = 0;


    HRESULT hres = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        flags,
        NULL,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &dx11SwapChain,
        &dx11Dev,
        NULL,
        &dx11Ctx );

    if( FAILED( hres ) )
    {
        SYS_NOT_IMPLEMENTED;
    }
	
	RDIDevice* device = BX_NEW( allocator, RDIDevice );
    RDICommandQueue* cmd_queue = &device->_immediate_command_queue;

	if( flags & D3D11_CREATE_DEVICE_DEBUG )
	{
		dx11Dev->QueryInterface( __uuidof(ID3D11Debug), reinterpret_cast<void**>(&device->_debug) );
        device->_debug->Release();
    }

	device->_device = dx11Dev;
    cmd_queue->_context = dx11Ctx;
    cmd_queue->_swapChain = dx11SwapChain;

    ID3D11Texture2D* backBuffer = NULL;
    hres = dx11SwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&backBuffer );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11RenderTargetView* viewRT = 0;
    hres = dx11Dev->CreateRenderTargetView( backBuffer, NULL, &viewRT );
    SYS_ASSERT( SUCCEEDED( hres ) );
    cmd_queue->_mainFramebuffer = viewRT;

    D3D11_TEXTURE2D_DESC bbdesc;
    backBuffer->GetDesc( &bbdesc );
    cmd_queue->_mainFramebufferWidth = bbdesc.Width;
    cmd_queue->_mainFramebufferHeight = bbdesc.Height;

    backBuffer->Release();

	dev[0] = device;
	cmdq[0] = cmd_queue;
}

static void Release( RDICommandQueue* cmdq )
{
    cmdq->_mainFramebuffer->Release();
    cmdq->_context->Release();
    cmdq->_swapChain->Release();
}

void ShutdownDX11( RDIDevice** dev, RDICommandQueue** cmdq, BXIAllocator* allocator )
{
    cmdq[0] = nullptr;

    Release( &dev[0]->_immediate_command_queue );
    dev[0]->dx11()->Release();
	if( dev[0]->_debug )
	{
		dev[0]->_debug->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
	}
    
	BX_DELETE0( allocator, dev[0] );
}

static inline uint32_t CalcHash( const char* name )
{
	const uint32_t HASH_SEED = 'I' << 24 | 'D' << 16 | '3' << 8 | 'D';
	return murmur3_hash32( name, (uint32_t)strlen( name ), HASH_SEED );
}

void Dx11FetchShaderReflection( RDIShaderReflection* out, const void* code_blob, size_t code_blob_size, int stage )
{
    ID3D11ShaderReflection* reflector = NULL;
    D3DReflect( code_blob, code_blob_size, IID_ID3D11ShaderReflection, (void**)&reflector );

    D3D11_SHADER_DESC sdesc;
    reflector->GetDesc( &sdesc );
#if 0
    for( uint32_t i = 0; i < sdesc.ConstantBuffers; ++i )
    {
        ID3D11ShaderReflectionConstantBuffer* cbuffer_reflector = reflector->GetConstantBufferByIndex( i );

        D3D11_SHADER_BUFFER_DESC sb_desc;
        cbuffer_reflector->GetDesc( &sb_desc );

        if( sb_desc.Name[0] == '_' )
            continue;

        D3D11_SHADER_INPUT_BIND_DESC bind_desc;
        reflector->GetResourceBindingDescByName( sb_desc.Name, &bind_desc );

        const uint32_t cb_hashed_name = CalcHash( sb_desc.Name );
		auto cbuffer_found = std::find_if( out->buffers.begin(), out->buffers.end(), 
			[cb_hashed_name]( const ShaderBufferDesc& desc ) -> bool { return desc.hashed_name == cb_hashed_name; } );
        if( cbuffer_found != out->buffers.end() )
        {
            cbuffer_found->stage_mask |= ( 1 << stage );
            continue;
        }

        out->buffers.push_back( ShaderBufferDesc() );
        ShaderBufferDesc& cb = out->buffers.back();
        cb.name = sb_desc.Name;
        cb.hashed_name = cb_hashed_name;
        cb.size = sb_desc.Size;
        cb.slot = bind_desc.BindPoint;
        cb.stage_mask = ( 1 << stage );
        cb.flags.cbuffer = 1;


        for( uint32_t j = 0; j < sb_desc.Variables; ++j )
        {
            ID3D11ShaderReflectionVariable* var_reflector = cbuffer_reflector->GetVariableByIndex( j );
            ID3D11ShaderReflectionType* var_reflector_type = var_reflector->GetType();
            D3D11_SHADER_VARIABLE_DESC sv_desc;
            D3D11_SHADER_TYPE_DESC typeDesc;

            var_reflector->GetDesc( &sv_desc );
            var_reflector_type->GetDesc( &typeDesc );

            cb.variables.push_back( ShaderVariableDesc() );
            ShaderVariableDesc& vdesc = cb.variables.back();
            vdesc.name = sv_desc.Name;
            vdesc.hashed_name = CalcHash( sv_desc.Name );
            vdesc.offset = sv_desc.StartOffset;
            vdesc.size = sv_desc.Size;
            vdesc.type = RDIEType::FindBaseType( typeDesc.Name );
        }

    }
#endif

    for( uint32_t i = 0; i < sdesc.BoundResources; ++i )
    {
        D3D11_SHADER_INPUT_BIND_DESC rdesc;
        reflector->GetResourceBindingDesc( i, &rdesc );

        // system variable
        if( rdesc.Name[0] == '_' )
            continue;

        const uint32_t hashed_name = CalcHash( rdesc.Name );

        if(
            rdesc.Type == D3D_SIT_CBUFFER ||
            rdesc.Type == D3D_SIT_UAV_RWTYPED ||
            rdesc.Type == D3D_SIT_STRUCTURED ||
            rdesc.Type == D3D_SIT_UAV_RWSTRUCTURED ||
            rdesc.Type == D3D_SIT_BYTEADDRESS ||
            rdesc.Type == D3D_SIT_UAV_RWBYTEADDRESS ||
            rdesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED ||
            rdesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED )
        {
            D3D11_SHADER_INPUT_BIND_DESC bind_desc;
            reflector->GetResourceBindingDescByName( rdesc.Name, &bind_desc );

            auto buffer_found = std::find_if( out->buffers.begin(), out->buffers.end(),
                [hashed_name]( const ShaderBufferDesc& desc ) -> bool { return desc.hashed_name == hashed_name; } );
            
            if( buffer_found != out->buffers.end() )
            {
                buffer_found->stage_mask |= (1 << stage);
                continue;
            }

            out->buffers.push_back( ShaderBufferDesc() );
            ShaderBufferDesc& cb = out->buffers.back();
            cb.name = rdesc.Name;
            cb.hashed_name = hashed_name;
            cb.slot = bind_desc.BindPoint;
            cb.stage_mask = (1 << stage);
            
            cb.flags.cbuffer = rdesc.Type == D3D_SIT_CBUFFER;
            cb.flags.structured = 
                rdesc.Type == D3D_SIT_STRUCTURED || rdesc.Type == D3D_SIT_UAV_RWSTRUCTURED ||
                rdesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED || rdesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED;
            cb.flags.byteaddress = 
                rdesc.Type == D3D_SIT_BYTEADDRESS ||
                rdesc.Type == D3D_SIT_UAV_RWBYTEADDRESS;
            cb.flags.uav = 
                rdesc.Type == D3D_SIT_UAV_RWTYPED || 
                rdesc.Type == D3D_SIT_UAV_RWSTRUCTURED ||
                rdesc.Type == D3D_SIT_UAV_RWBYTEADDRESS ||
                rdesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED ||
                rdesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED;
            cb.flags.read_write = 
                rdesc.Type == D3D_SIT_UAV_RWTYPED ||
                rdesc.Type == D3D_SIT_UAV_RWBYTEADDRESS;
            cb.flags.append = rdesc.Type == D3D_SIT_UAV_APPEND_STRUCTURED;
            cb.flags.consume = rdesc.Type == D3D_SIT_UAV_CONSUME_STRUCTURED;

            if( rdesc.Type == D3D_SIT_CBUFFER )
            {
                ID3D11ShaderReflectionConstantBuffer* cbuffer_reflector = reflector->GetConstantBufferByName( rdesc.Name );
                D3D11_SHADER_BUFFER_DESC sb_desc;
                cbuffer_reflector->GetDesc( &sb_desc );
                cb.size = sb_desc.Size;
            }
        }
        else if( rdesc.Type == D3D_SIT_TEXTURE )
        {
			auto found = std::find_if( out->textures.begin(), out->textures.end(), 
				[hashed_name]( const ShaderTextureDesc& desc ) -> bool { return desc.hashed_name == hashed_name; } );
            if( found != out->textures.end() )
            {
                found->stage_mask |= ( 1 << stage );
                continue;
            }
            out->textures.push_back( ShaderTextureDesc() );
            ShaderTextureDesc& tdesc = out->textures.back();
            tdesc.name = rdesc.Name;
			tdesc.hashed_name = hashed_name;
            tdesc.slot = rdesc.BindPoint;
            tdesc.stage_mask = ( 1 << stage );
            switch( rdesc.Dimension )
            {
            case D3D_SRV_DIMENSION_TEXTURE1D:
                tdesc.dimm = 1;
                break;
            case D3D_SRV_DIMENSION_TEXTURE2D:
                tdesc.dimm = 2;
                break;
            case D3D_SRV_DIMENSION_TEXTURE3D:
                tdesc.dimm = 3;
                break;
            case D3D_SRV_DIMENSION_TEXTURECUBE:
                tdesc.dimm = 2;
                tdesc.is_cubemap = 1;
                break;
            default:
                tdesc.dimm = 1;
                break;
            }
        }
        else if( rdesc.Type == D3D_SIT_SAMPLER )
        {
			auto found = std::find_if( out->samplers.begin(), out->samplers.end(),
				[hashed_name]( const ShaderSamplerDesc& desc ) -> bool { return desc.hashed_name == hashed_name; } );
			if( found != out->samplers.end() )
            {
                found->stage_mask |= ( 1 << stage );
                continue;
            }
            out->samplers.push_back( ShaderSamplerDesc() );
            ShaderSamplerDesc& sdesc = out->samplers.back();
            sdesc.name = rdesc.Name;
            sdesc.hashed_name = CalcHash( rdesc.Name );
            sdesc.slot = rdesc.BindPoint;
            sdesc.stage_mask = ( 1 << stage );
        }
    }

    if( stage == RDIEPipeline::VERTEX )
    {
        uint16_t input_mask = 0;
        RDIVertexLayout& layout = out->vertex_layout;
        layout.count = 0;

        for( uint32_t i = 0; i < sdesc.InputParameters; ++i )
        {
            D3D11_SIGNATURE_PARAMETER_DESC idesc;
            reflector->GetInputParameterDesc( i, &idesc );

            RDIEVertexSlot::Enum slot = RDIEVertexSlot::FromString( idesc.SemanticName );
            if( slot >= RDIEVertexSlot::COUNT )
            {
                continue;
            }
            RDIVertexBufferDesc& desc = layout.descs[layout.count++];

            input_mask |= ( 1 << ( slot + idesc.SemanticIndex ) );

            desc.slot = slot + idesc.SemanticIndex;
            desc.numElements = bitcount( (uint32_t)idesc.Mask );
            desc.typeNorm = 0;
            if( idesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32 )
            {
                desc.dataType = RDIEType::INT;
            }
            else if( idesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32 )
            {
                desc.dataType = RDIEType::UINT;
            }
            else if( idesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32 )
            {
                desc.dataType = RDIEType::FLOAT;
            }
            else
            {
                SYS_NOT_IMPLEMENTED;
            }
        }
        out->input_mask = input_mask;
    }
    reflector->Release();
}

}}///

using namespace bx::rdi;

void ReportLiveObjects( RDIDevice* dev )
{
	dev->_debug->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
}

RDIVertexBuffer CreateVertexBuffer( RDIDevice* dev, const RDIVertexBufferDesc& desc, uint32_t numElements, const void* data )
{
    const uint32_t stride = desc.ByteWidth();
    const uint32_t total_size = numElements * stride;
    D3D11_BUFFER_DESC bdesc;
    memset( &bdesc, 0, sizeof( bdesc ) );
    bdesc.ByteWidth = total_size;
    bdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    if( desc.gpuAccess )
    {
        if( desc.gpuAccess == RDIEGpuAccess::READ )
        {
            bdesc.Usage = D3D11_USAGE_IMMUTABLE;
            bdesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        }
        else
        {
            bdesc.Usage = D3D11_USAGE_DEFAULT;
            bdesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
        }
    }
    else
    {
        if( !desc.cpuAccess )
        {
            bdesc.Usage = D3D11_USAGE_IMMUTABLE;
            bdesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
        }
        else
        {
            if( desc.cpuAccess == RDIECpuAccess::WRITE )
            {
                bdesc.Usage = D3D11_USAGE_DYNAMIC;
                bdesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            }
            else
            {
                bdesc.BindFlags = 0;
                bdesc.Usage = D3D11_USAGE_STAGING;
            }
        }
    }

    if( bdesc.BindFlags & D3D11_BIND_SHADER_RESOURCE )
    {
        bdesc.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    }

    bdesc.CPUAccessFlags = to_D3D11_CPU_ACCESS_FLAG( desc.cpuAccess );;

    D3D11_SUBRESOURCE_DATA resource_data;
    resource_data.pSysMem = data;
    resource_data.SysMemPitch = 0;
    resource_data.SysMemSlicePitch = 0;

    D3D11_SUBRESOURCE_DATA* resource_data_pointer = (data) ? &resource_data : 0;

    ID3D11Buffer* buffer = 0;
    HRESULT hres = dev->dx11()->CreateBuffer( &bdesc, resource_data_pointer, &buffer );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11ShaderResourceView* viewSH = nullptr;
    ID3D11UnorderedAccessView* viewUA = nullptr;

    if( bdesc.BindFlags & D3D11_BIND_SHADER_RESOURCE )
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC descSH;
        memset( &descSH, 0x00, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );

        descSH.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        descSH.BufferEx.FirstElement = 0;
        descSH.Format = DXGI_FORMAT_R32_TYPELESS;
        descSH.BufferEx.NumElements = total_size / 4;
        descSH.BufferEx.Flags |= D3D11_BUFFEREX_SRV_FLAG_RAW;

        hres = dev->dx11()->CreateShaderResourceView( buffer, &descSH, &viewSH );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    if( bdesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS )
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC descUA;
        memset( &descUA, 0x00, sizeof( D3D11_UNORDERED_ACCESS_VIEW_DESC ) );

        descUA.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
        descUA.Format = DXGI_FORMAT_R32_TYPELESS;
        descUA.Buffer.NumElements = total_size / 4;
        descUA.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW;

        hres = dev->dx11()->CreateUnorderedAccessView( buffer, &descUA, &viewUA );
    }
    
    RDIVertexBuffer vBuffer;
    vBuffer.buffer      = buffer;
    vBuffer.numElements = numElements;
    vBuffer.desc        = desc;
    vBuffer.viewSH      = viewSH;
    vBuffer.viewUA      = viewUA;
    return vBuffer;
}
RDIIndexBuffer CreateIndexBuffer( RDIDevice* dev,RDIEType::Enum dataType, uint32_t numElements, const void* data )
{
    const DXGI_FORMAT d11_data_type = to_DXGI_FORMAT( RDIFormat( dataType, 1 ) );
    const uint32_t stride = RDIEType::stride[dataType];
    const uint32_t sizeInBytes = numElements * stride;

    D3D11_BUFFER_DESC bdesc;
    memset( &bdesc, 0, sizeof( bdesc ) );
    bdesc.ByteWidth      = sizeInBytes;
    bdesc.Usage          = ( data ) ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DYNAMIC;
    bdesc.BindFlags      = D3D11_BIND_INDEX_BUFFER;
    bdesc.CPUAccessFlags = ( data ) ? 0 : D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA resourceData;
    resourceData.pSysMem          = data;
    resourceData.SysMemPitch      = 0;
    resourceData.SysMemSlicePitch = 0;

    D3D11_SUBRESOURCE_DATA* resourceDataPointer = ( data ) ? &resourceData : 0;

    ID3D11Buffer* buffer = 0;
    HRESULT hres = dev->dx11()->CreateBuffer( &bdesc, resourceDataPointer, &buffer );
    SYS_ASSERT( SUCCEEDED( hres ) );

    RDIIndexBuffer iBuffer;
    iBuffer.buffer      = buffer;
    iBuffer.dataType    = dataType;
    iBuffer.numElements = numElements;

    return iBuffer;
}
RDIConstantBuffer CreateConstantBuffer( RDIDevice* dev,uint32_t sizeInBytes, const void* data )
{
    D3D11_BUFFER_DESC bdesc;
    memset( &bdesc, 0, sizeof( bdesc ) );
    bdesc.ByteWidth      = TYPE_ALIGN( sizeInBytes, 16 );
    bdesc.Usage          = D3D11_USAGE_DEFAULT;
    bdesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    bdesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA resourceData;
    resourceData.pSysMem          = data;
    resourceData.SysMemPitch      = 0;
    resourceData.SysMemSlicePitch = 0;

    D3D11_SUBRESOURCE_DATA* resourceDataPointer = ( data ) ? &resourceData : 0;

    ID3D11Buffer* buffer = 0;
    HRESULT hres = dev->dx11()->CreateBuffer( &bdesc, resourceDataPointer, &buffer );
    SYS_ASSERT( SUCCEEDED( hres ) );

	RDIConstantBuffer b;
    b.buffer        = buffer;
    b.size_in_bytes = sizeInBytes;
    return b;
}
RDIBufferRO CreateBufferRO( RDIDevice* dev,int numElements, RDIFormat format, unsigned cpuAccessFlag )
{
    const uint32_t dxBindFlags = D3D11_BIND_SHADER_RESOURCE;
    const uint32_t dxCpuAccessFlag = to_D3D11_CPU_ACCESS_FLAG( cpuAccessFlag );

    D3D11_USAGE dxUsage = D3D11_USAGE_DEFAULT;
    if( cpuAccessFlag & RDIECpuAccess::WRITE )
    {
        dxUsage = D3D11_USAGE_DYNAMIC;
    }

    const uint32_t formatByteWidth = format.ByteWidth();
    SYS_ASSERT( formatByteWidth > 0 );

    D3D11_BUFFER_DESC bdesc;
    memset( &bdesc, 0, sizeof( bdesc ) );
    bdesc.ByteWidth      = numElements * formatByteWidth;
    bdesc.Usage          = dxUsage;
    bdesc.BindFlags      = dxBindFlags;
    bdesc.CPUAccessFlags = dxCpuAccessFlag;

    ID3D11Buffer* buffer = 0;
    HRESULT hres = dev->dx11()->CreateBuffer( &bdesc, 0, &buffer );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11ShaderResourceView* viewSH = 0;

    {
        const DXGI_FORMAT dxFormat = to_DXGI_FORMAT( format );

        D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
        memset( &srvdesc, 0, sizeof( srvdesc ) );
        srvdesc.Format                = dxFormat;
        srvdesc.ViewDimension         = D3D11_SRV_DIMENSION_BUFFEREX;
        srvdesc.BufferEx.FirstElement = 0;
        srvdesc.BufferEx.NumElements  = numElements;
        srvdesc.BufferEx.Flags        = 0;
        hres = dev->dx11()->CreateShaderResourceView( buffer, &srvdesc, &viewSH );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

	RDIBufferRO b;
    b.buffer      = buffer;
    b.viewSH      = viewSH;
    b.sizeInBytes = bdesc.ByteWidth;
    b.format      = format;
    b.elementStride = format.ByteWidth();
    return b;
}

RDIBufferRO CreateStructuredBufferRO( RDIDevice* dev, uint32_t numElements, uint32_t elementStride, unsigned cpuAccessFlag )
{
    const uint32_t dxBindFlags = D3D11_BIND_SHADER_RESOURCE;
    const uint32_t dxCpuAccessFlag = to_D3D11_CPU_ACCESS_FLAG( cpuAccessFlag );

    D3D11_USAGE dxUsage = D3D11_USAGE_DEFAULT;
    if( cpuAccessFlag & RDIECpuAccess::WRITE )
    {
        dxUsage = D3D11_USAGE_DYNAMIC;
    }

    SYS_ASSERT( elementStride > 0 );

    D3D11_BUFFER_DESC bdesc;
    memset( &bdesc, 0, sizeof( bdesc ) );
    bdesc.ByteWidth = numElements * elementStride;
    bdesc.Usage = dxUsage;
    bdesc.BindFlags = dxBindFlags;
    bdesc.CPUAccessFlags = dxCpuAccessFlag;
    bdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bdesc.StructureByteStride = elementStride;

    ID3D11Buffer* buffer = 0;
    HRESULT hres = dev->dx11()->CreateBuffer( &bdesc, 0, &buffer );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11ShaderResourceView* viewSH = 0;

    {
        const DXGI_FORMAT dxFormat = DXGI_FORMAT_UNKNOWN;

        D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
        memset( &srvdesc, 0, sizeof( srvdesc ) );
        srvdesc.Format = dxFormat;
        srvdesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        srvdesc.Buffer.NumElements = numElements;
        hres = dev->dx11()->CreateShaderResourceView( buffer, &srvdesc, &viewSH );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    RDIBufferRO b;
    b.buffer = buffer;
    b.viewSH = viewSH;
    b.sizeInBytes = bdesc.ByteWidth;
    b.format = {};
    b.elementStride = elementStride;
    return b;
}

RDIBufferRO CreateRawBufferRO( RDIDevice* dev, uint32_t size_in_bytes, const void* initial_data, unsigned cpuAccessFlag )
{
    const uint32_t dxBindFlags = D3D11_BIND_SHADER_RESOURCE;
    const uint32_t dxCpuAccessFlag = to_D3D11_CPU_ACCESS_FLAG( cpuAccessFlag );

    D3D11_USAGE dxUsage = D3D11_USAGE_DEFAULT;
    if( cpuAccessFlag & RDIECpuAccess::WRITE )
    {
        dxUsage = D3D11_USAGE_DYNAMIC;
    }

    SYS_ASSERT( size_in_bytes > 0 );

    D3D11_BUFFER_DESC bdesc;
    memset( &bdesc, 0, sizeof( bdesc ) );
    bdesc.ByteWidth = size_in_bytes;
    bdesc.Usage = dxUsage;
    bdesc.BindFlags = dxBindFlags;
    bdesc.CPUAccessFlags = dxCpuAccessFlag;
    bdesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
    bdesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA resourceData;
    resourceData.pSysMem = initial_data;
    resourceData.SysMemPitch = 0;
    resourceData.SysMemSlicePitch = 0;

    D3D11_SUBRESOURCE_DATA* resourceDataPointer = (initial_data) ? &resourceData : 0;

    ID3D11Buffer* buffer = 0;
    HRESULT hres = dev->dx11()->CreateBuffer( &bdesc, resourceDataPointer, &buffer );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11ShaderResourceView* viewSH = 0;

    if( bdesc.BindFlags & D3D11_BIND_SHADER_RESOURCE )
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC descSH;
        memset( &descSH, 0x00, sizeof( D3D11_SHADER_RESOURCE_VIEW_DESC ) );

        descSH.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
        descSH.BufferEx.FirstElement = 0;
        descSH.Format = DXGI_FORMAT_R32_TYPELESS;
        descSH.BufferEx.NumElements = size_in_bytes / 4;
        descSH.BufferEx.Flags |= D3D11_BUFFEREX_SRV_FLAG_RAW;

        hres = dev->dx11()->CreateShaderResourceView( buffer, &descSH, &viewSH );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    RDIBufferRO b;
    b.buffer = buffer;
    b.viewSH = viewSH;
    b.sizeInBytes = bdesc.ByteWidth;
    b.format = {};
    b.elementStride = 8;
    return b;
}

namespace
{
    static void CreateVertexShader( RDIShaderPass* pass, RDIShaderReflection* reflection, RDIDevice* dev, const void* bytecode, size_t bytecode_size )
    {
        if( !bytecode || !bytecode_size )
            return;

        HRESULT hres;
        hres = dev->dx11()->CreateVertexShader( bytecode, bytecode_size, nullptr, &pass->vertex );
        SYS_ASSERT( SUCCEEDED( hres ) );

        ID3DBlob* blob = nullptr;
        hres = D3DGetInputSignatureBlob( bytecode, bytecode_size, &blob );
        SYS_ASSERT( SUCCEEDED( hres ) );
        pass->input_signature = (void*)blob;
        if( reflection )
        {
            Dx11FetchShaderReflection( reflection, bytecode, bytecode_size, RDIEPipeline::VERTEX );
            pass->vertex_input_mask = reflection->input_mask;
        }
    }
    static void CreatePixelShader( RDIShaderPass* pass, RDIShaderReflection* reflection, RDIDevice* dev, const void* bytecode, size_t bytecode_size )
    {
        if( !bytecode || !bytecode_size )
            return;

        HRESULT hres;

        hres = dev->dx11()->CreatePixelShader( bytecode, bytecode_size, nullptr, &pass->pixel );
        SYS_ASSERT( SUCCEEDED( hres ) );
        if( reflection )
        {
            Dx11FetchShaderReflection( reflection, bytecode, bytecode_size, RDIEPipeline::PIXEL );
        }
    }
    static void CreateComputeShader( RDIShaderPass* pass, RDIShaderReflection* reflection, RDIDevice* dev, const void* bytecode, size_t bytecode_size )
    {
        if( !bytecode || !bytecode_size )
            return;

        HRESULT hres;

        hres = dev->dx11()->CreateComputeShader( bytecode, bytecode_size, nullptr, &pass->compute );
        SYS_ASSERT( SUCCEEDED( hres ) );
        if( reflection )
        {
            Dx11FetchShaderReflection( reflection, bytecode, bytecode_size, RDIEPipeline::PIXEL );
        }
    }
}//

RDIShaderPass CreateShaderPass( RDIDevice* dev,const RDIShaderPassCreateInfo& info )
{
    RDIShaderPass pass = {};

    ::CreateVertexShader ( &pass, info.reflection, dev, info.bytecode[RDIEPipeline::VERTEX], info.bytecode_size[RDIEPipeline::VERTEX] );
    ::CreatePixelShader  ( &pass, info.reflection, dev, info.bytecode[RDIEPipeline::PIXEL], info.bytecode_size[RDIEPipeline::PIXEL] );
    ::CreateComputeShader( &pass, info.reflection, dev, info.bytecode[RDIEPipeline::COMPUTE], info.bytecode_size[RDIEPipeline::COMPUTE] );

    return pass;
}

RDITextureRO CreateTextureFromDDS( RDIDevice* dev,const void* dataBlob, size_t dataBlobSize )
{
    ID3D11Resource* resource = 0;
    ID3D11ShaderResourceView* srv = 0;

    DirectX::ScratchImage scratch_img = {};
    DirectX::TexMetadata tex_metadata = {};

    HRESULT hres = DirectX::LoadFromDDSMemory( dataBlob, dataBlobSize, DirectX::DDS_FLAGS_NONE, &tex_metadata, scratch_img );
    SYS_ASSERT( SUCCEEDED( hres ) );
    
    RDITextureRO tex;
    hres = DirectX::CreateTexture( dev->dx11(), scratch_img.GetImages(), scratch_img.GetImageCount(), tex_metadata, &tex.resource );
    SYS_ASSERT( SUCCEEDED( hres ) );

    hres = DirectX::CreateShaderResourceView( dev->dx11(), scratch_img.GetImages(), scratch_img.GetImageCount(), tex_metadata, &tex.viewSH );
    SYS_ASSERT( SUCCEEDED( hres ) );

    tex.info.width  = (uint16_t)tex_metadata.width;
    tex.info.height = (uint16_t)tex_metadata.height;
    tex.info.depth  = (uint16_t)tex_metadata.depth;
    tex.info.mips   = (uint8_t)tex_metadata.mipLevels;
    
    return tex;
}
RDITextureRO CreateTextureFromHDR( RDIDevice* dev,const void* dataBlob, size_t dataBlobSize )
{
    ID3D11Resource* resource = 0;
    ID3D11ShaderResourceView* srv = 0;

    DirectX::ScratchImage scratch_img = {};
    DirectX::TexMetadata tex_metadata = {};

    HRESULT hres = DirectX::LoadFromHDRMemory( dataBlob, dataBlobSize, &tex_metadata, scratch_img );
    SYS_ASSERT( SUCCEEDED( hres ) );

    RDITextureRO tex;
    hres = DirectX::CreateTexture( dev->dx11(), scratch_img.GetImages(), scratch_img.GetImageCount(), tex_metadata, &tex.resource );
    SYS_ASSERT( SUCCEEDED( hres ) );

    hres = DirectX::CreateShaderResourceView( dev->dx11(), scratch_img.GetImages(), scratch_img.GetImageCount(), tex_metadata, &tex.viewSH );
    SYS_ASSERT( SUCCEEDED( hres ) );

    tex.info.width  = (uint16_t)tex_metadata.width;
    tex.info.height = (uint16_t)tex_metadata.height;
    tex.info.depth  = (uint16_t)tex_metadata.depth;
    tex.info.mips   = (uint8_t)tex_metadata.mipLevels;
    return tex;
}


RDITextureRW CreateTexture1D( RDIDevice* dev,int w, int mips, RDIFormat format, unsigned bindFlags, unsigned cpuaFlags, const void* data )
{
    const DXGI_FORMAT dx_format  = to_DXGI_FORMAT( format );
    const uint32_t dx_bind_flags = to_D3D11_BIND_FLAG( bindFlags );
    const uint32_t dx_cpua_flags = to_D3D11_CPU_ACCESS_FLAG( cpuaFlags );

    D3D11_TEXTURE1D_DESC desc;
    memset( &desc, 0, sizeof( desc ) );
    desc.Width     = w;
    desc.MipLevels = mips;
    desc.ArraySize = 1;
    desc.Format    = dx_format;

    desc.Usage          = D3D11_USAGE_DEFAULT;
    desc.BindFlags      = dx_bind_flags;
    desc.CPUAccessFlags = dx_cpua_flags;

    if( desc.MipLevels > 1 )
    {
        desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    D3D11_SUBRESOURCE_DATA* subResourcePtr = 0;
    D3D11_SUBRESOURCE_DATA subResource;
    memset( &subResource, 0, sizeof( D3D11_SUBRESOURCE_DATA ) );
    if( data )
    {
        subResource.pSysMem          = data;
        subResource.SysMemPitch      = w * format.ByteWidth();
        subResource.SysMemSlicePitch = 0;
        subResourcePtr               = &subResource;
    }

    ID3D11Texture1D* tex1D = 0;
    HRESULT hres = dev->dx11()->CreateTexture1D( &desc, subResourcePtr, &tex1D );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11ShaderResourceView* view_sh  = 0;
    ID3D11RenderTargetView* view_rt    = 0;
    ID3D11UnorderedAccessView* view_ua = 0;

    ID3D11RenderTargetView** rtv_mips = 0;

    if( dx_bind_flags & D3D11_BIND_SHADER_RESOURCE )
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
        memset( &srvdesc, 0, sizeof( srvdesc ) );
        srvdesc.Format                    = dx_format;
        srvdesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE1D;
        srvdesc.Texture1D.MostDetailedMip = 0;
        srvdesc.Texture1D.MipLevels       = desc.MipLevels;
        hres                              = dev->dx11()->CreateShaderResourceView( tex1D, &srvdesc, &view_sh );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    if( dx_bind_flags & D3D11_BIND_RENDER_TARGET )
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;
        memset( &rtvdesc, 0, sizeof( rtvdesc ) );
        rtvdesc.Format             = dx_format;
        rtvdesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE1D;
        rtvdesc.Texture1D.MipSlice = 0;
        hres                       = dev->dx11()->CreateRenderTargetView( tex1D, &rtvdesc, &view_rt );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    if( dx_bind_flags & D3D11_BIND_DEPTH_STENCIL )
    {
        SYS_LOG_ERROR( "Depth stencil binding is not supported without depth texture format" );
    }

    if( dx_bind_flags & D3D11_BIND_UNORDERED_ACCESS )
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavdesc;
        memset( &uavdesc, 0, sizeof( uavdesc ) );
        uavdesc.Format             = dx_format;
        uavdesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavdesc.Texture1D.MipSlice = 0;
        hres                       = dev->dx11()->CreateUnorderedAccessView( tex1D, &uavdesc, &view_ua );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    RDITextureRW tex;
    tex.texture1D   = tex1D;
    tex.viewSH      = view_sh;
    tex.viewUA      = view_ua;
    tex.viewRT      = view_rt;
    tex.info.width  = w;
    tex.info.height = 1;
    tex.info.depth  = 1;
    tex.info.mips   = mips;
    tex.info.format = format;
    return tex;
}
RDITextureRW CreateTexture2D( RDIDevice* dev,int w, int h, int mips, RDIFormat format, unsigned bindFlags, unsigned cpuaFlags, const void* data )
{
    const DXGI_FORMAT dx_format = to_DXGI_FORMAT( format );
    const uint32_t dx_bind_flags = to_D3D11_BIND_FLAG( bindFlags );
    const uint32_t dx_cpua_flags = to_D3D11_CPU_ACCESS_FLAG( cpuaFlags );

    D3D11_TEXTURE2D_DESC desc;
    memset( &desc, 0, sizeof( desc ) );
    desc.Width              = w;
    desc.Height             = h;
    desc.MipLevels          = mips;
    desc.ArraySize          = 1;
    desc.Format             = dx_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage              = D3D11_USAGE_DEFAULT;
    desc.BindFlags          = dx_bind_flags;
    desc.CPUAccessFlags     = dx_cpua_flags;

    if( desc.MipLevels > 1 )
    {
        desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }

    D3D11_SUBRESOURCE_DATA* subResourcePtr = 0;
    D3D11_SUBRESOURCE_DATA subResource;
    memset( &subResource, 0, sizeof( D3D11_SUBRESOURCE_DATA ) );
    if( data )
    {
        subResource.pSysMem          = data;
        subResource.SysMemPitch      = w * format.ByteWidth();
        subResource.SysMemSlicePitch = 0;
        subResourcePtr               = &subResource;
    }

    ID3D11Texture2D* tex2D = 0;
    HRESULT hres = dev->dx11()->CreateTexture2D( &desc, subResourcePtr, &tex2D );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11ShaderResourceView* view_sh  = 0;
    ID3D11RenderTargetView* view_rt    = 0;
    ID3D11UnorderedAccessView* view_ua = 0;

    ID3D11RenderTargetView** rtv_mips = 0;

    if( dx_bind_flags & D3D11_BIND_SHADER_RESOURCE )
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
        memset( &srvdesc, 0, sizeof( srvdesc ) );
        srvdesc.Format                    = dx_format;
        srvdesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvdesc.Texture2D.MostDetailedMip = 0;
        srvdesc.Texture2D.MipLevels       = desc.MipLevels;
        hres                              = dev->dx11()->CreateShaderResourceView( tex2D, &srvdesc, &view_sh );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    if( dx_bind_flags & D3D11_BIND_RENDER_TARGET )
    {
        D3D11_RENDER_TARGET_VIEW_DESC rtvdesc;
        memset( &rtvdesc, 0, sizeof( rtvdesc ) );
        rtvdesc.Format             = dx_format;
        rtvdesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
        rtvdesc.Texture2D.MipSlice = 0;
        hres                       = dev->dx11()->CreateRenderTargetView( tex2D, &rtvdesc, &view_rt );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    if( dx_bind_flags & D3D11_BIND_DEPTH_STENCIL )
    {
        SYS_LOG_ERROR( "Depth stencil binding is not supported without depth texture format" );
    }

    if( dx_bind_flags & D3D11_BIND_UNORDERED_ACCESS )
    {
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavdesc;
        memset( &uavdesc, 0, sizeof( uavdesc ) );
        uavdesc.Format             = dx_format;
        uavdesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
        uavdesc.Texture2D.MipSlice = 0;
        hres                       = dev->dx11()->CreateUnorderedAccessView( tex2D, &uavdesc, &view_ua );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    RDITextureRW tex;
    tex.texture2D   = tex2D;
    tex.viewSH      = view_sh;
    tex.viewUA      = view_ua;
    tex.viewRT      = view_rt;
    tex.info.width  = w;
    tex.info.height = h;
    tex.info.depth  = 1;
    tex.info.mips   = mips;
    tex.info.format = format;
    return tex;
}
RDITextureDepth CreateTexture2Ddepth( RDIDevice* dev,int w, int h, int mips, RDIEType::Enum dataType )
{
    const DXGI_FORMAT dx_format = to_DXGI_FORMAT( RDIFormat( dataType, 1 ) );
    const uint32_t dx_bind_flags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // bx::gdi::to_D3D11_BIND_FLAG( bindFlags );

    SYS_ASSERT( isDepthFormat( dx_format ) );

    DXGI_FORMAT srv_format;
    DXGI_FORMAT tex_format;
    switch( dx_format )
    {
    case DXGI_FORMAT_D16_UNORM:
        tex_format = DXGI_FORMAT_R16_TYPELESS;
        srv_format = DXGI_FORMAT_R16_UNORM;
        break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        tex_format = DXGI_FORMAT_R24G8_TYPELESS;
        srv_format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        break;
    case DXGI_FORMAT_D32_FLOAT:
        tex_format = DXGI_FORMAT_R32_TYPELESS;
        srv_format = DXGI_FORMAT_R32_FLOAT;
        break;
    default:
        SYS_ASSERT( false );
        break;
    }

    D3D11_TEXTURE2D_DESC desc;
    memset( &desc, 0, sizeof( desc ) );
    desc.Width              = w;
    desc.Height             = h;
    desc.MipLevels          = mips;
    desc.ArraySize          = 1;
    desc.Format             = tex_format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage              = D3D11_USAGE_DEFAULT;
    desc.BindFlags          = dx_bind_flags;
    desc.CPUAccessFlags     = 0;

    ID3D11Texture2D* tex2d = 0;
    HRESULT hres = dev->dx11()->CreateTexture2D( &desc, 0, &tex2d );
    SYS_ASSERT( SUCCEEDED( hres ) );

    ID3D11ShaderResourceView* view_sh = 0;
    ID3D11DepthStencilView* view_ds = 0;

    if( dx_bind_flags & D3D11_BIND_SHADER_RESOURCE )
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvdesc;
        memset( &srvdesc, 0, sizeof( srvdesc ) );
        srvdesc.Format = srv_format;
        srvdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvdesc.Texture2D.MostDetailedMip = 0;
        srvdesc.Texture2D.MipLevels = mips;
        hres = dev->dx11()->CreateShaderResourceView( tex2d, &srvdesc, &view_sh );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    if( dx_bind_flags & D3D11_BIND_DEPTH_STENCIL )
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC dsvdesc;
        memset( &dsvdesc, 0, sizeof( dsvdesc ) );
        dsvdesc.Format = dx_format;
        dsvdesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        dsvdesc.Texture2D.MipSlice = 0;
        hres = dev->dx11()->CreateDepthStencilView( tex2d, &dsvdesc, &view_ds );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    RDITextureDepth tex = {};
    tex.texture2D   = tex2d;
    tex.viewDS      = view_ds;
    tex.viewSH      = view_sh;
    tex.info.width  = w;
    tex.info.height = h;
    tex.info.depth  = 1;
    tex.info.mips   = mips;
    tex.info.format = RDIFormat( dataType, 1 );

    return tex;
}
RDISampler CreateSampler( RDIDevice* dev,const RDISamplerDesc& desc )
{
    D3D11_SAMPLER_DESC dxDesc;
    dxDesc.Filter = ( desc.depthCmpMode == RDIESamplerDepthCmp::NONE ) ? filters[desc.filter] : comparisionFilters[desc.filter];
    dxDesc.ComparisonFunc = comparision[desc.depthCmpMode];

    dxDesc.AddressU = addressMode[desc.addressU];
    dxDesc.AddressV = addressMode[desc.addressV];
    dxDesc.AddressW = addressMode[desc.addressT];

    dxDesc.MaxAnisotropy = ( hasAniso( (RDIESamplerFilter::Enum )desc.filter ) ) ? desc.aniso : 1;

    {
        dxDesc.BorderColor[0] = 0.f;
        dxDesc.BorderColor[1] = 0.f;
        dxDesc.BorderColor[2] = 0.f;
        dxDesc.BorderColor[3] = 0.f;
    }
    dxDesc.MipLODBias = 0.f;
    dxDesc.MinLOD = 0;
    dxDesc.MaxLOD = D3D11_FLOAT32_MAX;
    //desc.MaxLOD = ( SamplerFilter::has_mipmaps((SamplerFilter::Enum)state.filter) ) ? D3D11_FLOAT32_MAX : 0;

    ID3D11SamplerState* dxState = 0;
    HRESULT hres = dev->dx11()->CreateSamplerState( &dxDesc, &dxState );
    SYS_ASSERT( SUCCEEDED( hres ) );

	RDISampler result;
    result.state = dxState;
    return result;
}

namespace 
{
    ID3D11InputLayout* _CreateInputLayoutInternal( RDIDevice* dev,const RDIVertexBufferDesc* blocks, int nblocks, const void* signature )
    {
        const int MAX_IEDESCS = cRDI_MAX_VERTEX_BUFFERS;
        SYS_ASSERT( nblocks < MAX_IEDESCS );

        D3D11_INPUT_ELEMENT_DESC d3d_iedescs[MAX_IEDESCS];
        for( int iblock = 0; iblock < nblocks; ++iblock )
        {
            const RDIVertexBufferDesc block = blocks[iblock];
            const RDIFormat format = RDIFormat( ( RDIEType::Enum )block.dataType, block.numElements ).Normalized( block.typeNorm );
            D3D11_INPUT_ELEMENT_DESC& d3d_desc = d3d_iedescs[iblock];

            d3d_desc.SemanticName = RDIEVertexSlot::name[block.slot];
            d3d_desc.SemanticIndex = RDIEVertexSlot::semanticIndex[block.slot];
            d3d_desc.Format = to_DXGI_FORMAT( format );
            d3d_desc.InputSlot = iblock;
            d3d_desc.AlignedByteOffset = 0;
            d3d_desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
            d3d_desc.InstanceDataStepRate = 0;
        }

        ID3D11InputLayout *dxLayout = 0;
        ID3DBlob* signatureBlob = (ID3DBlob*)signature;
        HRESULT hres = dev->dx11()->CreateInputLayout( d3d_iedescs, nblocks, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), &dxLayout );
        SYS_ASSERT( SUCCEEDED( hres ) );

        return dxLayout;
    }
}

RDIInputLayout CreateInputLayout( RDIDevice* dev,const RDIVertexLayout BXVertexLayout, RDIShaderPass shaderPass )
{
	RDIInputLayout iLay;
    iLay.layout = _CreateInputLayoutInternal( dev, BXVertexLayout.descs, BXVertexLayout.count, shaderPass.input_signature );
    return iLay;
}

RDIHardwareState CreateHardwareState( RDIDevice* dev, RDIHardwareStateDesc desc )
{
	RDIHardwareState hwstate = {};
    {
		RDIHardwareStateDesc::Blend state = desc.blend;
        D3D11_BLEND_DESC bdesc;
        memset( &bdesc, 0, sizeof( bdesc ) );

        bdesc.AlphaToCoverageEnable          = FALSE;
        bdesc.IndependentBlendEnable         = FALSE;
        bdesc.RenderTarget[0].BlendEnable    = state.enable;
        bdesc.RenderTarget[0].SrcBlend       = blendFactor[state.srcFactor];
        bdesc.RenderTarget[0].DestBlend      = blendFactor[state.dstFactor];
        bdesc.RenderTarget[0].BlendOp        = blendEquation[state.equation];
        bdesc.RenderTarget[0].SrcBlendAlpha  = blendFactor[state.srcFactorAlpha];
        bdesc.RenderTarget[0].DestBlendAlpha = blendFactor[state.dstFactorAlpha];
        bdesc.RenderTarget[0].BlendOpAlpha   = blendEquation[state.equation];

        uint8_t mask = 0;
        mask |= ( state.color_mask & RDIEColorMask::RED ) ? D3D11_COLOR_WRITE_ENABLE_RED : 0;
        mask |= ( state.color_mask & RDIEColorMask::GREEN ) ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0;
        mask |= ( state.color_mask & RDIEColorMask::BLUE ) ? D3D11_COLOR_WRITE_ENABLE_BLUE : 0;
        mask |= ( state.color_mask & RDIEColorMask::ALPHA ) ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0;

        bdesc.RenderTarget[0].RenderTargetWriteMask = mask;

        ID3D11BlendState* dx_state = 0;
        HRESULT hres = dev->dx11()->CreateBlendState( &bdesc, &hwstate.blend );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    {
		RDIHardwareStateDesc::Depth state = desc.depth;
        D3D11_DEPTH_STENCIL_DESC dsdesc;
        memset( &dsdesc, 0, sizeof( dsdesc ) );

        dsdesc.DepthEnable    = state.test;
        dsdesc.DepthWriteMask = ( state.write ) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        dsdesc.DepthFunc      = depthCmpFunc[state.function];

        dsdesc.StencilEnable    = FALSE;
        dsdesc.StencilReadMask  = D3D11_DEFAULT_STENCIL_READ_MASK;
        dsdesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;

        dsdesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
        dsdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsdesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        dsdesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;

        dsdesc.BackFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
        dsdesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        dsdesc.BackFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        dsdesc.BackFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;

        ID3D11DepthStencilState* dx_state = 0;
        HRESULT hres = dev->dx11()->CreateDepthStencilState( &dsdesc, &hwstate.depth );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    {
		RDIHardwareStateDesc::Raster state = desc.raster;
        D3D11_RASTERIZER_DESC rdesc;
        memset( &rdesc, 0, sizeof( rdesc ) );

        rdesc.FillMode              = fillMode[state.fillMode];
        rdesc.CullMode              = cullMode[state.cullMode];
        rdesc.FrontCounterClockwise = TRUE;
        rdesc.DepthBias             = 0;
        rdesc.SlopeScaledDepthBias  = 0.f;
        rdesc.DepthBiasClamp        = 0.f;
        rdesc.DepthClipEnable       = TRUE;
        rdesc.ScissorEnable         = state.scissor;
        rdesc.MultisampleEnable     = FALSE;
        rdesc.AntialiasedLineEnable = state.antialiasedLine;

        ID3D11RasterizerState* dx_state = 0;
        HRESULT hres = dev->dx11()->CreateRasterizerState( &rdesc, &hwstate.raster );
        SYS_ASSERT( SUCCEEDED( hres ) );
    }

    return hwstate;
}



template< class T >
static inline void releaseSafe( T*& obj )
{
    if( obj )
    {
        obj->Release();
        obj = nullptr;
    }
}

void Destroy( RDIVertexBuffer* id )
{
    releaseSafe( id->viewUA );
    releaseSafe( id->viewSH );
    releaseSafe( id->buffer );
}
void Destroy( RDIIndexBuffer* id )
{
    releaseSafe( id->buffer );
}
void Destroy( RDIInputLayout * id )
{
    releaseSafe( id->layout );
}
void Destroy( RDIConstantBuffer* id )
{
    releaseSafe( id->buffer );
}
void Destroy( RDIBufferRO* id )
{
    releaseSafe( id->viewSH );
    releaseSafe( id->buffer );
}
void Destroy( RDIShaderPass* id )
{
    releaseSafe( id->vertex );
    releaseSafe( id->pixel );
    releaseSafe( id->compute );

    if( id->input_signature )
    {
        ID3DBlob* blob = (ID3DBlob*)id->input_signature;
        blob->Release();
        id->input_signature = nullptr;
    }
}

void Destroy( RDITextureRO* id )
{
    releaseSafe( id->viewSH );
    releaseSafe( id->resource );
}
void Destroy( RDITextureRW* id )
{
    releaseSafe( id->viewSH );
    releaseSafe( id->viewRT );
    releaseSafe( id->viewUA );
    releaseSafe( id->resource );
}
void Destroy( RDITextureDepth* id )
{
    releaseSafe( id->viewDS );
    releaseSafe( id->viewSH );
    releaseSafe( id->viewUA );
    releaseSafe( id->resource );
}
void Destroy( RDISampler* id )
{
    releaseSafe( id->state );
}
void Destroy( RDIBlendState* id )
{
    releaseSafe( id->state );
}
void Destroy( RDIDepthState* id )
{
    releaseSafe( id->state );
}
void Destroy( RDIRasterState * id )
{
    releaseSafe( id->state );
}

void Destroy( RDIHardwareState* id )
{
    releaseSafe( id->raster );
    releaseSafe( id->depth );
    releaseSafe( id->blend );
}

//---
void GetAPIDevice( RDIDevice* dev,ID3D11Device** apiDev, ID3D11DeviceContext** apiCtx )
{
	apiDev[0] = dev->dx11();
    apiCtx[0] = dev->_immediate_command_queue._context;
}
RDICommandQueue* GetImmediateCommandQueue( RDIDevice* dev )
{
    return &dev->_immediate_command_queue;
}

void SetViewport( RDICommandQueue* cmdq, RDIViewport vp )
{
    D3D11_VIEWPORT dxVp;

    dxVp.Width = (FLOAT)vp.w;
    dxVp.Height = (FLOAT)vp.h;
    dxVp.MinDepth = 0.0f;
    dxVp.MaxDepth = 1.0f;
    dxVp.TopLeftX = (FLOAT)vp.x;
    dxVp.TopLeftY = (FLOAT)vp.y;

    cmdq->dx11()->RSSetViewports( 1, &dxVp );
}
void SetVertexBuffers( RDICommandQueue* cmdq, RDIVertexBuffer* vbuffers, unsigned start, unsigned n )
{
    ID3D11Buffer* buffers[cRDI_MAX_VERTEX_BUFFERS];
    unsigned strides[cRDI_MAX_VERTEX_BUFFERS];
    unsigned offsets[cRDI_MAX_VERTEX_BUFFERS];
    memset( buffers, 0, sizeof( buffers ) );
    memset( strides, 0, sizeof( strides ) );
    memset( offsets, 0, sizeof( offsets ) );

    for( unsigned i = 0; i < n; ++i )
    {
        if( !vbuffers[i].buffer )
            continue;

        const RDIVertexBuffer& buffer = vbuffers[i];
        strides[i] = buffer.desc.ByteWidth();
        buffers[i] = buffer.buffer;
    }
    cmdq->dx11()->IASetVertexBuffers( start, cRDI_MAX_VERTEX_BUFFERS, buffers, strides, offsets );
}
void SetIndexBuffer( RDICommandQueue* cmdq, RDIIndexBuffer ibuffer )
{
    DXGI_FORMAT format = ( ibuffer.buffer ) ? to_DXGI_FORMAT( RDIFormat( (RDIEType::Enum)ibuffer.dataType, 1 ) ) : DXGI_FORMAT_UNKNOWN;
    cmdq->dx11()->IASetIndexBuffer( ibuffer.buffer, format, 0 );
}

void SetShaderPass( RDICommandQueue* cmdq, RDIShaderPass pass )
{
    cmdq->dx11()->VSSetShader( pass.vertex, 0, 0 );
    cmdq->dx11()->PSSetShader( pass.pixel, 0, 0 );
    cmdq->dx11()->CSSetShader( pass.compute, 0, 0 );
}

void SetInputLayout( RDICommandQueue* cmdq, RDIInputLayout ilay )
{
    cmdq->dx11()->IASetInputLayout( ilay.layout );
}

void SetCbuffers( RDICommandQueue* cmdq, RDIConstantBuffer* cbuffers, unsigned startSlot, unsigned n, unsigned stageMask )
{
    const int SLOT_COUNT = cRDI_MAX_CBUFFERS;
    SYS_ASSERT( n <= SLOT_COUNT );
    ID3D11Buffer* buffers[SLOT_COUNT];
    memset( buffers, 0, sizeof( buffers ) );

    if( cbuffers )
    {
        for( unsigned i = 0; i < n; ++i )
        {
            buffers[i] = cbuffers[i].buffer;
        }
    }

    if( stageMask & RDIEPipeline::VERTEX_MASK )
        cmdq->dx11()->VSSetConstantBuffers( startSlot, n, buffers );

    if( stageMask & RDIEPipeline::PIXEL_MASK )
        cmdq->dx11()->PSSetConstantBuffers( startSlot, n, buffers );

    if( stageMask & RDIEPipeline::COMPUTE_MASK )
        cmdq->dx11()->CSSetConstantBuffers( startSlot, n, buffers );
}
void SetResourcesRO( RDICommandQueue* cmdq, RDIResourceRO* resources, unsigned startSlot, unsigned n, unsigned stageMask )
{
    const int SLOT_COUNT = cRDI_MAX_RESOURCES_RO;
    SYS_ASSERT( n <= SLOT_COUNT );

    ID3D11ShaderResourceView* views[SLOT_COUNT];
    memset( views, 0, sizeof( views ) );

    if( resources )
    {
        for( unsigned i = 0; i < n; ++i )
        {
            views[i] = resources[i].viewSH;
        }
    }

    if( stageMask & RDIEPipeline::VERTEX_MASK )
        cmdq->dx11()->VSSetShaderResources( startSlot, n, views );

    if( stageMask & RDIEPipeline::PIXEL_MASK )
        cmdq->dx11()->PSSetShaderResources( startSlot, n, views );

    if( stageMask & RDIEPipeline::COMPUTE_MASK )
        cmdq->dx11()->CSSetShaderResources( startSlot, n, views );
}
void SetResourcesRW( RDICommandQueue* cmdq, RDIResourceRW* resources, unsigned startSlot, unsigned n, unsigned stageMask )
{
    const int SLOT_COUNT = cRDI_MAX_RESOURCES_RW;
    SYS_ASSERT( n <= SLOT_COUNT );

    ID3D11UnorderedAccessView* views[SLOT_COUNT];
    UINT initial_count[SLOT_COUNT] = {};

    memset( views, 0, sizeof( views ) );

    if( resources )
    {
        for( unsigned i = 0; i < n; ++i )
        {
            views[i] = resources[i].viewUA;
        }
    }

    if( stageMask & (RDIEPipeline::VERTEX_MASK || RDIEPipeline::PIXEL_MASK) )
    {
        SYS_LOG_ERROR( "ResourceRW can be set only in compute stage" );
    }

    if( stageMask & RDIEPipeline::COMPUTE_MASK )
        cmdq->dx11()->CSSetUnorderedAccessViews( startSlot, n, views, initial_count );
}
void SetSamplers( RDICommandQueue* cmdq, RDISampler* samplers, unsigned startSlot, unsigned n, unsigned stageMask )
{
    const int SLOT_COUNT = cRDI_MAX_SAMPLERS;
    SYS_ASSERT( n <= SLOT_COUNT );
    ID3D11SamplerState* resources[SLOT_COUNT];

    for( unsigned i = 0; i < n; ++i )
    {
        resources[i] = samplers[i].state;
    }

    if( stageMask & RDIEPipeline::VERTEX_MASK )
        cmdq->dx11()->VSSetSamplers( startSlot, n, resources );

    if( stageMask & RDIEPipeline::PIXEL_MASK )
        cmdq->dx11()->PSSetSamplers( startSlot, n, resources );

    if( stageMask & RDIEPipeline::COMPUTE_MASK )
        cmdq->dx11()->CSSetSamplers( startSlot, n, resources );
}

void SetDepthState( RDICommandQueue* cmdq, RDIDepthState state )
{
    cmdq->dx11()->OMSetDepthStencilState( state.state, 0 );
}
void SetBlendState( RDICommandQueue* cmdq, RDIBlendState state )
{
    const float bfactor[4] = { 0.f, 0.f, 0.f, 0.f };
    cmdq->dx11()->OMSetBlendState( state.state, bfactor, 0xFFFFFFFF );
}
void SetRasterState( RDICommandQueue* cmdq, RDIRasterState state )
{
    cmdq->dx11()->RSSetState( state.state );
}
void SetHardwareState( RDICommandQueue* cmdq, RDIHardwareState hwstate )
{
    cmdq->dx11()->OMSetDepthStencilState( hwstate.depth, 0 );
    
    const float bfactor[4] = { 0.f, 0.f, 0.f, 0.f };
    cmdq->dx11()->OMSetBlendState( hwstate.blend, bfactor, 0xFFFFFFFF );
    
    cmdq->dx11()->RSSetState( hwstate.raster );
}

void SetScissorRects( RDICommandQueue* cmdq, const RDIRect* rects, int n )
{
    const int cMAX_RECTS = 4;
    D3D11_RECT dx11Rects[cMAX_RECTS];
    SYS_ASSERT( n <= cMAX_RECTS );

    for( int i = 0; i < cMAX_RECTS; ++i )
    {
        D3D11_RECT& r = dx11Rects[i];
        r.left   = rects[i].left;
        r.top    = rects[i].top;
        r.right  = rects[i].right;
        r.bottom = rects[i].bottom;
    }

    cmdq->dx11()->RSSetScissorRects( n, dx11Rects );
}
void SetTopology( RDICommandQueue* cmdq, int topology )
{
    cmdq->dx11()->IASetPrimitiveTopology( topologyMap[topology] );
}

void ChangeToMainFramebuffer( RDICommandQueue* cmdq )
{
    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.MinDepth = 0;
    vp.MaxDepth = 1;
    vp.Width    = (float)cmdq->_mainFramebufferWidth;
    vp.Height   = (float)cmdq->_mainFramebufferHeight;

    cmdq->dx11()->OMSetRenderTargets( 1, &cmdq->_mainFramebuffer, 0 );
    cmdq->dx11()->RSSetViewports( 1, &vp );
}
void ChangeRenderTargets( RDICommandQueue* cmdq, RDITextureRW* colorTex, unsigned nColor, RDITextureDepth depthTex, bool changeViewport )
{
    const int SLOT_COUNT = cRDI_MAX_RENDER_TARGETS;
    SYS_ASSERT( nColor <= SLOT_COUNT );

    ID3D11RenderTargetView *rtv[SLOT_COUNT] = {};
    ID3D11DepthStencilView *dsv = depthTex.viewDS;

    for( uint32_t i = 0; i < nColor; ++i )
    {
        rtv[i] = colorTex[i].viewRT;
    }

    cmdq->dx11()->OMSetRenderTargets( SLOT_COUNT, rtv, dsv );

    if( changeViewport )
    {
		RDIViewport vp;
        vp.x = 0;
        vp.y = 0;
        vp.w = ( colorTex ) ? colorTex->info.width  : depthTex.info.width;
        vp.h = ( colorTex ) ? colorTex->info.height : depthTex.info.height;
        SetViewport( cmdq, vp );
    }
}

static inline unsigned char* _MapResource( ID3D11DeviceContext* ctx, ID3D11Resource* resource, D3D11_MAP dxMapType, int offsetInBytes )
{
    D3D11_MAPPED_SUBRESOURCE mappedRes;
    mappedRes.pData      = NULL;
    mappedRes.RowPitch   = 0;
    mappedRes.DepthPitch = 0;

    ctx->Map( resource, 0, dxMapType, 0, &mappedRes );
    return (uint8_t*)mappedRes.pData + offsetInBytes;
}

static inline D3D11_MAP ToD3D11_MAP( RDIEMapType::Enum mapType )
{
    switch( mapType )
    {
    case RDIEMapType::WRITE           : return D3D11_MAP_WRITE_DISCARD;
    case RDIEMapType::WRITE_NO_DISCARD: return D3D11_MAP_WRITE;
    case RDIEMapType::READ            : return D3D11_MAP_READ;
    default:
        SYS_NOT_IMPLEMENTED;
        return D3D11_MAP_READ_WRITE;
    }
}

unsigned char* Map( RDICommandQueue* cmdq, RDIResource resource, int offsetInBytes, RDIEMapType::Enum mapType )
{
    const D3D11_MAP dxMapType = ToD3D11_MAP( mapType );
    return _MapResource( cmdq->dx11(), resource.resource, dxMapType, offsetInBytes );
}
void Unmap( RDICommandQueue* cmdq, RDIResource resource )
{
    cmdq->dx11()->Unmap( resource.resource, 0 );
}

unsigned char* Map( RDICommandQueue* cmdq, RDIVertexBuffer vbuffer, int firstElement, int numElements, RDIEMapType::Enum mapType )
{
    SYS_ASSERT( (uint32_t)( firstElement + numElements ) <= vbuffer.numElements );

    const int offsetInBytes = firstElement * vbuffer.desc.ByteWidth();
    const D3D11_MAP dxMapType = ToD3D11_MAP( mapType );

    return _MapResource( cmdq->dx11(), vbuffer.resource, dxMapType, offsetInBytes );
}
unsigned char* Map( RDICommandQueue* cmdq, RDIIndexBuffer ibuffer, int firstElement, int numElements, RDIEMapType::Enum mapType )
{
    SYS_ASSERT( (uint32_t)( firstElement + numElements ) <= ibuffer.numElements );
    SYS_ASSERT( ibuffer.dataType == RDIEType::USHORT || ibuffer.dataType == RDIEType::UINT );

    const int offsetInBytes = firstElement * RDIEType::stride[ibuffer.dataType];
    const D3D11_MAP dxMapType = ToD3D11_MAP( mapType );

    return _MapResource( cmdq->dx11(), ibuffer.resource, dxMapType, offsetInBytes );
}

void UpdateCBuffer( RDICommandQueue* cmdq, RDIConstantBuffer cbuffer, const void* data )
{
    cmdq->dx11()->UpdateSubresource( cbuffer.resource, 0, NULL, data, 0, 0 );
}
void UpdateTexture( RDICommandQueue* cmdq, RDITextureRW texture, const void* data )
{
    const uint32_t formatWidth   = texture.info.format.ByteWidth();
    const uint32_t srcRowPitch   = formatWidth * texture.info.width;
    const uint32_t srcDepthPitch = srcRowPitch * texture.info.height;
    cmdq->dx11()->UpdateSubresource( texture.resource, 0, NULL, data, srcRowPitch, srcDepthPitch );
}

void Draw( RDICommandQueue* cmdq, unsigned numVertices, unsigned startIndex )
{
    cmdq->dx11()->Draw( numVertices, startIndex );
}
void DrawIndexed( RDICommandQueue* cmdq, unsigned numIndices, unsigned startIndex, unsigned baseVertex )
{
    cmdq->dx11()->DrawIndexed( numIndices, startIndex, baseVertex );
}
void DrawInstanced( RDICommandQueue* cmdq, unsigned numVertices, unsigned startIndex, unsigned numInstances )
{
    cmdq->dx11()->DrawInstanced( numVertices, numInstances, startIndex, 0 );
}
void DrawIndexedInstanced( RDICommandQueue* cmdq, unsigned numIndices, unsigned startIndex, unsigned numInstances, unsigned baseVertex )
{
    cmdq->dx11()->DrawIndexedInstanced( numIndices, numInstances, startIndex, baseVertex, 0 );
}

void Dispatch( RDICommandQueue* cmdq, unsigned numGroupsX, unsigned numGroupsY, unsigned numGroupsZ )
{
    cmdq->dx11()->Dispatch( numGroupsX, numGroupsY, numGroupsZ );
}

void ClearState( RDICommandQueue* cmdq )
{
    cmdq->dx11()->ClearState();
}
void ClearBuffers( RDICommandQueue* cmdq, RDITextureRW* colorTex, unsigned nColor, RDITextureDepth depthTex, const float rgbad[5], int flag_color, int flag_depth )
{
    const int SLOT_COUNT = cRDI_MAX_RENDER_TARGETS;
    SYS_ASSERT( nColor < SLOT_COUNT );

    ID3D11RenderTargetView *rtv[SLOT_COUNT] = {};
    ID3D11DepthStencilView *dsv = depthTex.viewDS;
    for( uint32_t i = 0; i < nColor; ++i )
    {
        rtv[i] = colorTex[i].viewRT;
    }

    if( ( ( !colorTex || !nColor ) && !depthTex.resource ) && flag_color )
    {
        cmdq->dx11()->ClearRenderTargetView( cmdq->_mainFramebuffer, rgbad );
    }
    else
    {
        if( flag_depth && dsv )
        {
            cmdq->dx11()->ClearDepthStencilView( dsv, D3D11_CLEAR_DEPTH, rgbad[4], 0 );
        }

        if( flag_color && nColor )
        {
            for( unsigned i = 0; i < nColor; ++i )
            {
                cmdq->dx11()->ClearRenderTargetView( rtv[i], rgbad );
            }
        }
    }
}
void ClearDepthBuffer( RDICommandQueue* cmdq, RDITextureDepth depthTex, float clearValue )
{
    cmdq->dx11()->ClearDepthStencilView( depthTex.viewDS, D3D11_CLEAR_DEPTH, clearValue, 0 );
}
void ClearColorBuffers( RDICommandQueue* cmdq, RDITextureRW* colorTex, unsigned nColor, float r, float g, float b, float a )
{
    const float rgba[4] = { r, g, b, a };

    const int SLOT_COUNT = cRDI_MAX_RENDER_TARGETS;
    SYS_ASSERT( nColor < SLOT_COUNT );
    
    for( unsigned i = 0; i < nColor; ++i )
    {
        cmdq->dx11()->ClearRenderTargetView( colorTex[i].viewRT, rgba );
    }

}

void Swap( RDICommandQueue* cmdq, unsigned syncInterval )
{
    cmdq->_swapChain->Present( syncInterval, 0 );
}
void GenerateMipmaps( RDICommandQueue* cmdq, RDITextureRW texture )
{
    cmdq->dx11()->GenerateMips( texture.viewSH );
}
RDITextureRW GetBackBufferTexture( RDICommandQueue* cmdq )
{
    RDITextureRW tex;
    tex.id          = 0;
    tex.viewRT      = cmdq->_mainFramebuffer;
    tex.info.width  = cmdq->_mainFramebufferWidth;
    tex.info.height = cmdq->_mainFramebufferHeight;

    return tex;
}

