#pragma once

#include <foundation/type.h>
#include <foundation/debug.h>

namespace RDIEPipeline
{
    enum Enum
    {
        VERTEX = 0,
        PIXEL,
        COMPUTE,
        COUNT,
        DRAW_STAGES_COUNT = PIXEL + 1,

        ALL_STAGES_MASK = 0xFF,

        VERTEX_MASK  = BIT_OFFSET( VERTEX ),
        PIXEL_MASK   = BIT_OFFSET( PIXEL ),
        COMPUTE_MASK = BIT_OFFSET( COMPUTE ),
    };

    static const char* name[COUNT] =
    {
        "vertex",
        "pixel",
        "compute",
    };
};

namespace RDIEBind
{
    enum Enum
    {
        NONE = 0,
        VERTEX_BUFFER    = BIT_OFFSET( 0 ),
        INDEX_BUFFER     = BIT_OFFSET( 1 ),
        CONSTANT_BUFFER  = BIT_OFFSET( 2 ),
        SHADER_RESOURCE  = BIT_OFFSET( 3 ),
        STREAM_OUTPUT    = BIT_OFFSET( 4 ),
        RENDER_TARGET    = BIT_OFFSET( 5 ),
        DEPTH_STENCIL    = BIT_OFFSET( 6 ),
        UNORDERED_ACCESS = BIT_OFFSET( 7 ),
    };
};

namespace RDIEType
{
    enum Enum
    {
        UNKNOWN = 0,
        BYTE,
        UBYTE,
        SHORT,
        USHORT,
        INT,
        UINT,
        FLOAT,
        DOUBLE,
        DEPTH16,
        DEPTH24_STENCIL8,
        DEPTH32F,

        COUNT,
    };
    static const unsigned stride[] =
    {
        0, //
        1, //BYTE,
        1, //UBYTE,
        2, //SHORT,
        2, //USHORT,
        4, //INT,
        4, //UINT,
        4, //FLOAT,
        8, //DOUBLE,
        2, //DEPTH16,
        4, //DEPTH24_STENCIL8,
        4, //DEPTH32F,
    };
    static const char* name[] =
    {
        "none",
        "byte",
        "ubyte",
        "short",
        "ushort",
        "int",
        "uint",
        "float",
        "double",
        "depth16",
        "depth24_stencil8",
        "depth32F",
    };
    Enum FromName( const char* name );
    Enum FindBaseType( const char* name );

};


struct RDIFormat
{
    uint8_t type;
    uint8_t numElements : 6;
    uint8_t normalized : 1;
    uint8_t srgb : 1;

	RDIFormat()
        : type( RDIEType::UNKNOWN)
        , numElements(0), normalized(0), srgb(0)
    {}

    RDIFormat( RDIEType::Enum dt, uint8_t ne ) { type = dt; numElements = ne; normalized = 0; srgb = 0; }
    RDIFormat& Normalized( uint32_t onOff ) { normalized = onOff; return *this; }
    RDIFormat& Srgb( uint32_t onOff ) { srgb = onOff; return *this; }
    inline uint32_t ByteWidth() const { return RDIEType::stride[type] * numElements; }

	static RDIFormat Float()  { return RDIFormat( RDIEType::FLOAT, 1 ); }
	static RDIFormat Float2() { return RDIFormat( RDIEType::FLOAT, 2 ); }
	static RDIFormat Float3() { return RDIFormat( RDIEType::FLOAT, 3 ); }
	static RDIFormat Float4() { return RDIFormat( RDIEType::FLOAT, 4 ); }
};


namespace RDIECpuAccess
{
    enum Enum
    {
        READ = BIT_OFFSET( 0 ),
        WRITE = BIT_OFFSET( 1 ),
		READ_WRITE = READ | WRITE,
    };
};
namespace RDIEGpuAccess
{
    enum Enum
    {
        READ = BIT_OFFSET( 0 ),
        WRITE = BIT_OFFSET( 1 ),
        READ_WRITE = READ | WRITE,
    };
};

namespace RDIEMapType
{
    enum Enum
    {
        WRITE = 0,
        WRITE_NO_DISCARD,
        READ,
    };
};

namespace RDIETopology
{
    enum Enum
    {
        POINTS = 0,
        LINES,
        LINE_STRIP,
        TRIANGLES,
        TRIANGLE_STRIP,
    };
};

namespace RDIEVertexSlot
{
    enum Enum
    {
        POSITION = 0,
        NORMAL,
        COLOR0,
        COLOR1,
        TEXCOORD0,
        TEXCOORD1,
        TEXCOORD2,
        TEXCOORD3,
        TEXCOORD4,
        TEXCOORD5,
        TANGENT,
        BINORMAL,
        BLENDWEIGHT,
        BLENDINDICES,
        COUNT,
    };

    static const char* name[RDIEVertexSlot::COUNT] =
    {
        "POSITION",
        "NORMAL",
        "COLOR",
        "COLOR",
        "TEXCOORD",
        "TEXCOORD",
        "TEXCOORD",
        "TEXCOORD",
        "TEXCOORD",
        "TEXCOORD",
        "TANGENT",
        "BINORMAL",
        "BLENDWEIGHT",
        "BLENDINDICES",
    };
    static const int semanticIndex[RDIEVertexSlot::COUNT] =
    {
        0, //POSITION = 0
        0, //NORMAL,	
        0, //COLOR0,	
        1, //COLOR1,	
        0, //TEXCOORD0,	
        1, //TEXCOORD1,
        2, //TEXCOORD2,
        3, //TEXCOORD3,
        4, //TEXCOORD4,
        5, //TEXCOORD5,
        0, //TANGENT,
        0, //BINORMAL
        0, //BLENDWEIGHT,
        0, //BLENDINDICES
    };
	RDIEVertexSlot::Enum FromString( const char* n );
    bool ToString( char* output, uint32_t output_size, RDIEVertexSlot::Enum slot );

    inline uint32_t SkinningMaskPosNrm()
    {
        return BIT_OFFSET( POSITION ) | BIT_OFFSET( NORMAL );
    }

};


//////////////////////////////////////////////////////////////////////
/// hwState
namespace RDIEDepthFunc
{
    enum Enum
    {
        NEVER = 0,
        LESS,
        EQUAL,
        LEQUAL,
        GREATER,
        NOTEQUAL,
        GEQUAL,
        ALWAYS,
    };
};


namespace RDIEBlendFactor
{
    enum Enum
    {
        ZERO = 0,
        ONE,
        SRC_COLOR,
        ONE_MINUS_SRC_COLOR,
        DST_COLOR,
        ONE_MINUS_DST_COLOR,
        SRC_ALPHA,
        ONE_MINUS_SRC_ALPHA,
        DST_ALPHA,
        ONE_MINUS_DST_ALPHA,
        SRC_ALPHA_SATURATE,
    };
};

namespace RDIEBlendEquation
{
    enum Enum
    {
        ADD = 0,
        SUB,
        REVERSE_SUB,
        MIN,
        MAX,
    };
};

namespace RDIECullMode
{
    enum Enum
    {
        NONE = 0,
        BACK,
        FRONT,
    };
};

namespace RDIEFillMode
{
    enum Enum
    {
        SOLID = 0,
        WIREFRAME,
    };
};

namespace RDIEColorMask
{
    enum Enum
    {
        NONE = 0,
        RED = BIT_OFFSET( 0 ),
        GREEN = BIT_OFFSET( 1 ),
        BLUE = BIT_OFFSET( 2 ),
        ALPHA = BIT_OFFSET( 3 ),
        ALL = RED | GREEN | BLUE | ALPHA,
    };
};

namespace RDIESamplerFilter
{
    enum Enum
    {
        NEAREST = 0,
        LINEAR,
        BILINEAR,
        TRILINEAR,
        BILINEAR_ANISO,
        TRILINEAR_ANISO,
    };

    static const char* name[] =
    {
        "NEAREST",
        "LINEAR",
        "BILINEAR",
        "TRILINEAR",
        "BILINEAR_ANISO",
        "TRILINEAR_ANISO",
    };
    inline int hasMipmaps( Enum filter )	{ return ( filter >= BILINEAR ); }
    inline int hasAniso  ( Enum filter )    { return ( filter >= BILINEAR_ANISO ); }
};


namespace RDIESamplerDepthCmp
{
    enum Enum
    {
        NONE = 0,
        GREATER,
        GEQUAL,
        LESS,
        LEQUAL,
        ALWAYS,
        NEVER,
        COUNT
    };
};

namespace RDIEAddressMode
{
    enum Enum
    {
        WRAP,
        CLAMP,
        BORDER,
    };

    static const char* name[] =
    {
        "WRAP",
        "CLAMP",
        "BORDER",
    };
};


static const uint32_t cRDI_MAX_RENDER_TARGETS = 8;
static const uint32_t cRDI_MAX_CBUFFERS = 6;
static const uint32_t cRDI_MAX_TEXTURES = 8;
static const uint32_t cRDI_MAX_RESOURCES_RO = 8;
static const uint32_t cRDI_MAX_RESOURCES_RW = 6;
static const uint32_t cRDI_MAX_SAMPLERS = 8;
static const uint32_t cRDI_MAX_VERTEX_BUFFERS = RDIEVertexSlot::COUNT;
static const uint32_t cRDI_MAX_SHADER_MACRO = 32;

//////////////////////////////////////////////////////////////////////////
union RDISamplerDesc
{
	RDISamplerDesc()
        : filter  ( RDIESamplerFilter::TRILINEAR_ANISO )
        , addressU( RDIEAddressMode::WRAP )
        , addressV( RDIEAddressMode::WRAP )
        , addressT( RDIEAddressMode::WRAP )
        , depthCmpMode( RDIESamplerDepthCmp::NONE )
        , aniso( 16 )
    {}

    RDISamplerDesc& Filter  ( RDIESamplerFilter::Enum f )     { filter = f; return *this; }
    RDISamplerDesc& Address ( RDIEAddressMode::Enum u )       { addressU = addressV = addressT = u; return *this;  }
    RDISamplerDesc& Address ( RDIEAddressMode::Enum u, 
							  RDIEAddressMode::Enum v, 
							  RDIEAddressMode::Enum t )       { addressU = u;  addressV = v;  addressT = t; return *this; }
    RDISamplerDesc& DepthCmp( RDIESamplerDepthCmp::Enum dc )  { depthCmpMode = dc; return *this; }
    RDISamplerDesc& Aniso   ( uint8_t a )                          { aniso = a; return *this; }

    uint64_t key;
    struct
    {
        uint8_t filter;
        uint8_t addressU;
        uint8_t addressV;
        uint8_t addressT;
        uint8_t depthCmpMode;
        uint8_t aniso;
        uint16_t _padding;
    };
};

struct RDIHardwareStateDesc
{
    union Blend
    {
        Blend& Enable     ()																 { enable = 1; return *this; }
        Blend& Disable    ()																 { enable = 0; return *this; }
        Blend& ColorMask  ( RDIEColorMask::Enum m )									 { color_mask = m; return *this; }
        Blend& Factor     ( RDIEBlendFactor::Enum src, RDIEBlendFactor::Enum dst ) { srcFactor = src; dstFactor = dst; return *this; }
        Blend& FactorAlpha( RDIEBlendFactor::Enum src, RDIEBlendFactor::Enum dst ) { srcFactorAlpha = src; dstFactorAlpha = dst; return *this; }
        Blend& Equation   ( RDIEBlendEquation::Enum eq )								 { equation = eq; return *this; }

        uint32_t key;
        struct
        {
            uint64_t enable : 1;
            uint64_t color_mask : 4;
            uint64_t equation : 4;
            uint64_t srcFactor : 4;
            uint64_t dstFactor : 4;
            uint64_t srcFactorAlpha : 4;
            uint64_t dstFactorAlpha : 4;
        };
    };

    union Depth
    {
        Depth& Function( RDIEDepthFunc::Enum f ) { function = f; return *this; }
        Depth& Test    ( uint8_t onOff )           { test = onOff; return *this; }
        Depth& Write   ( uint8_t onOff )           { write = onOff; return *this; }

        uint32_t key;
        struct
        {
            uint16_t function;
            uint8_t  test;
            uint8_t	write;
        };
    };

    union Raster
    {
        Raster& CullMode        ( RDIECullMode::Enum c )  { cullMode = c; return *this; }
        Raster& FillMode        ( RDIEFillMode::Enum f ) { fillMode = f; return *this; }
        Raster& Multisample     ( uint32_t onOff )         { multisample = onOff; return *this; }
        Raster& AntialiassedLine( uint32_t onOff )         { antialiasedLine = onOff; return *this; }
        Raster& Scissor         ( uint32_t onOff )         { scissor = onOff; return *this; }
            
        uint32_t key;
        struct
        {
            uint32_t cullMode : 2;
            uint32_t fillMode : 2;
            uint32_t multisample : 1;
            uint32_t antialiasedLine : 1;
            uint32_t scissor : 1;
        };
    };

    RDIHardwareStateDesc()
    {
        blend
            .Disable    ()
            .ColorMask  ( RDIEColorMask::ALL )
            .FactorAlpha( RDIEBlendFactor::ONE, RDIEBlendFactor::ZERO )
            .Factor     ( RDIEBlendFactor::ONE, RDIEBlendFactor::ZERO )
            .Equation   ( RDIEBlendEquation::ADD );
            
        depth.Function( RDIEDepthFunc::LEQUAL ).Test( 1 ).Write( 1 );

        raster.CullMode( RDIECullMode::BACK ).FillMode( RDIEFillMode::SOLID ).Multisample( 1 ).AntialiassedLine( 1 ).Scissor( 0 );
    }
    Blend blend;
    Depth depth;
    Raster raster;
};

struct RDIViewport
{
	int16_t x, y;
	uint16_t w, h;

	static inline RDIViewport Create( const int32_t xywh[4] )
	{
		RDIViewport vp;
		vp.x = (int16_t)xywh[0];
		vp.y = (int16_t)xywh[1];
		vp.w = (uint16_t)xywh[2];
		vp.h = (uint16_t)xywh[3];
		return vp;
	}
};


//////////////////////////////////////////////////////////////////////////
union RDIVertexBufferDesc
{
    RDIVertexBufferDesc() {}
    RDIVertexBufferDesc( RDIEVertexSlot::Enum s ) { slot = s; }
    RDIVertexBufferDesc& DataType( RDIEType::Enum dt, uint32_t ne )
    { 
        dataType = dt; 
        numElements = ne;
        return *this; 
    }
    RDIVertexBufferDesc& Normalized() { typeNorm = 1; return *this; }
    RDIVertexBufferDesc& CPURead()  { cpuAccess = RDIECpuAccess::READ; return *this; }
    RDIVertexBufferDesc& CPUWrite() { cpuAccess = RDIECpuAccess::WRITE; return *this; }
    RDIVertexBufferDesc& GPURead()  { gpuAccess = RDIEGpuAccess::READ; return *this; }
    RDIVertexBufferDesc& GPUWrite() { gpuAccess = RDIEGpuAccess::WRITE; return *this; }

    inline uint32_t ByteWidth() const { return RDIEType::stride[dataType] * numElements; }

    uint16_t hash = 0;
    struct
    {
        uint16_t slot : 4;
        uint16_t dataType : 4;
        uint16_t typeNorm : 1;
        uint16_t numElements : 3;
        uint16_t cpuAccess : 2;
        uint16_t gpuAccess : 2;
    };

	static RDIVertexBufferDesc POS()  { return RDIVertexBufferDesc( RDIEVertexSlot::POSITION ).DataType( RDIEType::FLOAT, 3 ); }
    static RDIVertexBufferDesc POS4() { return RDIVertexBufferDesc( RDIEVertexSlot::POSITION ).DataType( RDIEType::FLOAT, 4 ); }
	static RDIVertexBufferDesc NRM()  { return RDIVertexBufferDesc( RDIEVertexSlot::NORMAL ).DataType( RDIEType::FLOAT, 3 ); }
    static RDIVertexBufferDesc TAN()  { return RDIVertexBufferDesc( RDIEVertexSlot::TANGENT ).DataType( RDIEType::FLOAT, 3 ); }
    static RDIVertexBufferDesc BIN()  { return RDIVertexBufferDesc( RDIEVertexSlot::BINORMAL ).DataType( RDIEType::FLOAT, 3 ); }
    static RDIVertexBufferDesc BW ()  { return RDIVertexBufferDesc( RDIEVertexSlot::BLENDWEIGHT ).DataType( RDIEType::FLOAT, 4 ); }
    static RDIVertexBufferDesc BI()   { return RDIVertexBufferDesc( RDIEVertexSlot::BLENDINDICES ).DataType( RDIEType::UBYTE, 4 ); }

    static RDIVertexBufferDesc UV( uint32_t set_index ) 
    { 
        SYS_ASSERT( set_index <= RDIEVertexSlot::TEXCOORD5 );
        return RDIVertexBufferDesc( (RDIEVertexSlot::Enum)(RDIEVertexSlot::TEXCOORD0 + set_index) ).DataType( RDIEType::FLOAT, 2 ); 
    }

    static RDIVertexBufferDesc UV0() { return RDIVertexBufferDesc( RDIEVertexSlot::TEXCOORD0 ).DataType( RDIEType::FLOAT, 2 ); }
};

struct RDIVertexLayout
{
    RDIVertexBufferDesc descs[cRDI_MAX_VERTEX_BUFFERS] = {};
    uint32_t count = 0;

    uint32_t InputMask() const
    {
        uint32_t mask = 0;
        for( uint32_t i = 0; i < count; ++i )
            mask |= 1 << descs[i].slot;
        return mask;
    }
};

//---
//////////////////////////////////////////////////////////////////////////
/// dx11 structs
struct ID3D11Device;
struct ID3D11DeviceContext;

struct ID3D11Resource;
struct ID3D11DeviceChild;
struct ID3D11Buffer;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11ComputeShader;
struct ID3D11InputLayout;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11SamplerState;

struct ID3D11Texture1D;
struct ID3D11Texture2D;
struct ID3D11Texture3D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11UnorderedAccessView;
//////////////////////////////////////////////////////////////////////////

struct RDIResource
{
	union
	{
		uintptr_t id = 0;
		ID3D11Resource*  resource;
		ID3D11Texture1D* texture1D;
		ID3D11Texture2D* texture2D;
		ID3D11Texture3D* texture3D;
		ID3D11Buffer*    buffer;
	};
};

struct RDIResourceRO : RDIResource
{
	ID3D11ShaderResourceView* viewSH = nullptr;
};
struct RDIResourceRW : RDIResourceRO
{
	ID3D11UnorderedAccessView* viewUA = nullptr;
};



struct RDIVertexBuffer : RDIResourceRW
{
	RDIVertexBufferDesc desc = {};
	uint32_t numElements = 0;
};
struct RDIIndexBuffer : RDIResource
{
	uint32_t dataType = 0;
	uint32_t numElements = 0;
};

struct RDIConstantBuffer : RDIResource
{
	uint32_t size_in_bytes = 0;
};

struct RDIBufferDesc
{
	uint32_t sizeInBytes = 0;
	uint32_t bind_flags = 0;
	RDIFormat format = {};
    uint16_t elementStride = 0;

	uint32_t NumElements() const { return sizeInBytes / format.ByteWidth(); }
};

struct RDIBufferRO : RDIResourceRO, RDIBufferDesc
{
};
struct RDIBufferRW : RDIResourceRW, RDIBufferDesc
{
};

struct RDITextureInfo
{
	uint16_t width = 0;
	uint16_t height = 0;
	uint16_t depth = 0;
	uint8_t mips = 0;
	RDIFormat format = {};

};

struct RDITextureRO : RDIResourceRO
{
	RDITextureInfo info;
};

struct RDITextureRW : RDIResourceRW
{
	ID3D11RenderTargetView* viewRT = nullptr;
	RDITextureInfo info;
};

struct RDITextureDepth : RDIResourceRW
{
	ID3D11DepthStencilView* viewDS = nullptr;
	RDITextureInfo info;
};

struct RDIShaderReflection;
struct RDIShaderPass
{
	ID3D11VertexShader* vertex = nullptr;
	ID3D11PixelShader* pixel = nullptr;
    ID3D11ComputeShader* compute = nullptr;

	void* input_signature = nullptr;
	uint32_t vertex_input_mask = 0;
};

struct RDIShaderPassCreateInfo
{
    void* bytecode[RDIEPipeline::COUNT] = {};
    size_t bytecode_size[RDIEPipeline::COUNT] = {};
	RDIShaderReflection* reflection = nullptr;
};

union RDIInputLayout
{
	uintptr_t id = 0;
	ID3D11InputLayout* layout;
};

union RDIBlendState
{
	uintptr_t id = 0;
	ID3D11BlendState* state;
};
union RDIDepthState
{
	uintptr_t id = 0;
	ID3D11DepthStencilState* state;
};
union RDIRasterState
{
	uintptr_t id = 0;
	ID3D11RasterizerState* state;
};

struct RDIHardwareState
{
	ID3D11BlendState* blend = nullptr;
	ID3D11DepthStencilState* depth = nullptr;
	ID3D11RasterizerState* raster = nullptr;
};

union RDISampler
{
	uintptr_t id = 0;
	ID3D11SamplerState* state;
};

struct RDIRect
{
	int16_t left, top, right, bottom;

	RDIRect() {}
	RDIRect( int l, int t, int r, int b )
		: left( l ), top( t ), right( r ), bottom( b )
	{}
};
