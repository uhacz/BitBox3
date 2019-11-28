#include "low_rendering.h"

#include "../../rdi_backend/rdi_backend.h"
#include "../../rdix/rdix.h"
#include "atomic"
#include "float16.h"
#include "algorithm"
#include "eastl/hash_map.h"
#include "container_allocator.h"


static constexpr u32 MAX_PIPELINE = 128;
static constexpr u32 MAX_GEOMETY = 1 << 16;
static constexpr u32 MAX_GEOMETRY_STREAMS = MAX_GEOMETY * 8;
static constexpr u32 MAX_INSTANCE = 1 << 16;
static constexpr u32 MAX_CAMERA = 32;
static constexpr u32 MAX_TARGET = 64;
static constexpr u32 MAX_DRAW_CMD = 1 << 17;

struct PipelineData
{
    RDIShaderPass shader;
    RDIXResourceBinding* resources;
    RDIHardwareStateDesc hw_state_desc;
    RDIETopology::Enum topology;
};
struct GeometryData
{
    RDIBufferRO vertices;
    RDIBufferRO insices;
    shader::DrawRange draw_range;

    u32 streams_index;
    u32 nb_streams;
    //shader::VertexStream streams[1];
};

struct InstanceData 
{
    u32 nb_instances;
    u32 __padding[3];
    mat44_t data[1];
};

union DrawCmdData
{
    struct  
    {
        u64 lo;
        u64 hi;
    } key;
    struct  
    {
        u64 index : 24;
        u64 depth : 16;
        u64 instance_offset : 24;
        u64 geometry_offset : 24;
        u64 pipeline_index : 16;
        u64 camera_index : 8;
        u64 target_index : 8;
        u64 layer : 8;
    };
};
static inline bool operator < ( const DrawCmdData& left, const DrawCmdData& right )
{
    return (left.key.hi == right.key.hi) ? left.key.lo < right.key.lo : left.key.hi < right.key.hi;
}


SYS_STATIC_ASSERT( sizeof( DrawCmdData ) == 16 );

using CameraData = shader::CameraViewData;
using TargetData = LowRenderer::Target;
using AtomicU32 = std::atomic_uint32_t;

static constexpr u32 SIZE_GEOMETRY_BUFFER = MAX_GEOMETY * sizeof( GeometryData );
static constexpr u32 SIZE_INSTANCE_BUFFER = MAX_INSTANCE * sizeof( InstanceData );
static constexpr u32 SIZE_GENERIC_BUFFER = 1024 * 64;

struct ManagedResources
{
    using HwStateMap = eastl::hash_map<u64, RDIHardwareState, eastl::hash<u64>, eastl::equal_to<u64>, bx_container_allocator>;
    HwStateMap hw_state_map;
};
RDIHardwareState AcquireHardwareState( RDIDevice* dev, ManagedResources* data, const RDIHardwareStateDesc& desc )
{
    auto it = data->hw_state_map.find( desc.hash );
    if( it == data->hw_state_map.end() )
    {
        RDIHardwareState hw_state = CreateHardwareState( dev, desc );
        data->hw_state_map[desc.hash] = hw_state;
        return hw_state;
    }
    else
    {
        return it->second;
    }
}

struct GpuResources
{
    RDIBufferRO target;
    RDIBufferRO camera;
    RDIBufferRO matrices;
    RDIConstantBuffer draw_call;

    static void Create( GpuResources* resources, RDIDevice* dev )
    {
        resources->target = CreateStructuredBufferRO( dev, MAX_TARGET, sizeof( shader::RenderTargetData ), RDIECpuAccess::WRITE );
        resources->camera = CreateStructuredBufferRO( dev, MAX_CAMERA, sizeof( shader::CameraViewData ), RDIECpuAccess::WRITE );
        resources->matrices = CreateStructuredBufferRO( dev, MAX_INSTANCE, sizeof( shader::float4x4 ), RDIECpuAccess::WRITE );
        resources->draw_call = CreateConstantBuffer( dev, sizeof( shader::DrawCallData ) );
    }
    static void Destroy( GpuResources* resources )
    {
        ::Destroy( &resources->draw_call );
        ::Destroy( &resources->matrices );
        ::Destroy( &resources->camera );
        ::Destroy( &resources->target );
    }
    static void Bind( GpuResources* resources, RDICommandQueue* cmdq )
    {
        SetResourcesRO( cmdq, &resources->target, 0, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, &resources->camera, 1, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, &resources->matrices, 2, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetCbuffers   ( cmdq, &resources->draw_call, 16, 1, RDIEPipeline::ALL_STAGES_MASK );
    }
};

struct LowRenderer::Impl
{
    ManagedResources managed_resources;
    GpuResources gpu_resources;

    PipelineData pipelines[MAX_PIPELINE];
    GeometryData geometry_data[MAX_GEOMETY];
    shader::VertexStream geometry_streams[MAX_GEOMETRY_STREAMS];

    //u8 geometry[SIZE_GEOMETRY_BUFFER];
    u8 instance[SIZE_INSTANCE_BUFFER];
    u8 generic_data[SIZE_GENERIC_BUFFER];
    CameraData camera[MAX_CAMERA];
    TargetData target[MAX_TARGET];
    DrawCmdData draw_cmd[MAX_DRAW_CMD];

    AtomicU32 nb_pipeline = 0;
    AtomicU32 nb_geometry = 0;
    AtomicU32 nb_geometry_streams = 0;
    //AtomicU32 offset_geometry = 0;
    AtomicU32 offset_instance = 0;
    AtomicU32 offset_generic = 0;
    AtomicU32 nb_camera = 0;
    AtomicU32 nb_target = 0;
    AtomicU32 nb_draw_cmd = 0;

    BXIAllocator* allocator = nullptr;
};

static void StartUp( LowRenderer::Impl* impl, RDIDevice* dev )
{
    GpuResources::Create( &impl->gpu_resources, dev );
}
static void ShutDown( LowRenderer::Impl* impl )
{
    GpuResources::Destroy( &impl->gpu_resources );
}


template< typename T >
struct AllocResult
{
    T* pointer;
    union
    {
        u32 offset;
        u32 index;
    };

    static AllocResult Null() { return { nullptr, UINT32_MAX }; }
    bool IsValid() const { return pointer && offset != UINT32_MAX; }

    T* operator -> () { return pointer; }
    operator bool () { return IsValid(); }
};

void* AllocGenericData( LowRenderer::Impl* impl, u32 size )
{
    const u32 offset = impl->offset_generic.fetch_add( size );
    if( offset + size > SIZE_GENERIC_BUFFER )
        return nullptr;

    return impl->generic_data + offset;
}

AllocResult<PipelineData> AllocPipeline( LowRenderer::Impl* impl )
{
    const u32 index = impl->nb_pipeline++;
    if( index >= MAX_PIPELINE )
        return AllocResult<PipelineData>::Null();

    return { &impl->pipelines[index], index };
}
AllocResult<GeometryData> AllocGeometry( LowRenderer::Impl* impl, u32 nb_streams )
{
    SYS_ASSERT( nb_streams > 0 );
    const u32 mem_required = sizeof( GeometryData ) + ((nb_streams - 1) * sizeof( GeometryData::streams ));
    
    const u32 offset = impl->offset_geometry.fetch_add( mem_required );
    if( offset + mem_required > SIZE_GEOMETRY_BUFFER )
    {
        return AllocResult<GeometryData>::Null();
    }

    return { (GeometryData*)(impl->geometry + offset ), offset };
}

AllocResult<InstanceData> AllocInstance( LowRenderer::Impl* impl, u32 nb_instances )
{
    const u32 required_mem = sizeof( InstanceData ) + ((nb_instances - 1) * sizeof( InstanceData::data[0] ));
    const u32 offset = impl->offset_instance.fetch_add( required_mem );
    if( offset + required_mem > SIZE_INSTANCE_BUFFER )
    {
        return AllocResult<InstanceData>::Null();
    }

    return { (InstanceData*)impl->instance + offset, offset };
}

AllocResult<CameraData> AllocCamera( LowRenderer::Impl* impl )
{
    const u32 index = impl->nb_camera++;
    if( index >= MAX_CAMERA )
        return AllocResult<CameraData>::Null();

    return { &impl->camera[index], index };
}
AllocResult<TargetData> AllocTarget( LowRenderer::Impl* impl )
{
    const u32 index = impl->nb_target++;
    if( index >= MAX_TARGET )
        return AllocResult<TargetData>::Null();

    return { &impl->target[index], index };
}
AllocResult<DrawCmdData> AllocDrawCmd( LowRenderer::Impl* impl )
{
    const u32 index = impl->nb_draw_cmd++;
    if( index >= MAX_DRAW_CMD )
        return AllocResult<DrawCmdData>::Null();

    return { &impl->draw_cmd[index], index };
}

u8 LowRenderer::AddTarget( const Target& target )
{
    AllocResult<TargetData> data = AllocTarget( impl );
    if( !data )
        return MAX_TARGET;

    SYS_STATIC_ASSERT( sizeof( TargetData ) == sizeof( Target ) );

    memcpy( data.pointer, &target, sizeof( TargetData ) );

    return data.index;
}

u8 LowRenderer::AddCamera( const Camera& camera )
{
    AllocResult<CameraData> data = AllocCamera( impl );
    if( !data )
        return MAX_CAMERA;

    data->world    = camera._matrices.world;
    data->view     = camera._matrices.view;
    data->proj     = camera._matrices.proj;
    data->proj_api = camera._matrices.proj_api;

    data->aspect = camera._params.aspect();
    data->fov    = camera._params.fov();
    data->znear  = camera._params.znear;
    data->zfar   = camera._params.zfar;

    return data.index;
}

u8 LowRenderer::AddPipeline( const Pipeline& pipeline )
{
    AllocResult<PipelineData> data = AllocPipeline( impl );
    if( !data )
        return MAX_PIPELINE;

    data->shader = pipeline._pipeline->pass;
    data->hw_state_desc = pipeline._hw_state_desc;
    data->topology = pipeline._topology;

    data->resources = nullptr;
    const RDIXResourceBinding* resources = (pipeline._resources) ? pipeline._resources : pipeline._pipeline->resources;
    const u32 resources_mem_size = GetMemorySize( resources );
    if( resources_mem_size )
    {
        data->resources = (RDIXResourceBinding*)AllocGenericData( impl, resources_mem_size );
        if( data->resources )
        {
            CloneResourceBinding( data->resources, resources_mem_size, resources );
        }
        else
        {
            SYS_LOG_WARNING( "Failed to allocate data for pipeline resources" );
        }
    }

    return data.index;
}

u32 LowRenderer::AddGeometry( const Geometry& geometry )
{
    SYS_ASSERT( geometry._vertex_layout.nb_streams <= MAX_VERTEX_STREAMS );

    AllocResult<GeometryData> data = AllocGeometry( impl, geometry._vertex_layout.nb_streams );
    if( !data )
        return SIZE_GEOMETRY_BUFFER;

    data->vertices = geometry._vertices;
    data->insices = geometry._indices;
    data->draw_range = geometry._range;
    data->nb_streams = geometry._vertex_layout.nb_streams;
    for( u32 i = 0; i < geometry._vertex_layout.nb_streams; ++i )
        data->streams[i] = geometry._vertex_layout.stream[i];

    return data.index;
}

u32 LowRenderer::AddInstance( const Instance& instance )
{
    AllocResult<InstanceData> data = AllocInstance( impl, instance._count );
    if( !data )
        return SIZE_INSTANCE_BUFFER;

    SYS_ASSERT( instance._stride == sizeof( InstanceData::data[0] ) );

    memcpy( data->data, instance._data, instance._count * instance._stride );
    data->nb_instances = instance._count;

    return data.offset;
}

bool LowRenderer::AddDrawCmd( const DrawCmd& cmd, f32 depth )
{
    AllocResult<DrawCmdData> data = AllocDrawCmd( impl );

    if( !data )
        return false;

    data->key.lo = 0;
    data->key.hi = 0;

    data->index = data.index;
    data->depth = float_to_half_fast2( fromF32( depth ) ).u;
    data->instance_offset = cmd.instance_offset;
    data->geometry_offset = cmd.geometry_offset;
    data->pipeline_index = cmd.pipeline_index;
    data->camera_index = cmd.camera_index;
    data->target_index = cmd.target_index;
    data->layer = cmd.layer;

    return true;
}

static inline bool ValidateDrawCmdData( const DrawCmdData& cmd )
{
    bool result = true;
    result = result && (cmd.target_index < MAX_TARGET);
    result = result && (cmd.camera_index < MAX_CAMERA);
    result = result && (cmd.pipeline_index < MAX_PIPELINE);
    result = result && (cmd.geometry_offset < SIZE_GEOMETRY_BUFFER);
    result = result && (cmd.instance_offset < SIZE_INSTANCE_BUFFER);
    return result;
}

void LowRenderer::Flush( RDICommandQueue* cmdq, RDIDevice* dev )
{
    if( impl->nb_draw_cmd == 0 )
        return;

    {
        shader::RenderTargetData* shader_target = (shader::RenderTargetData*)Map( cmdq, impl->gpu_resources.target, RDIEMapType::WRITE );
        shader::CameraViewData* shader_camera = (shader::CameraViewData*)Map( cmdq, impl->gpu_resources.camera, RDIEMapType::WRITE );
        shader::float4x4* shader_matrices = (shader::float4x4*)Map( cmdq, impl->gpu_resources.matrices, RDIEMapType::WRITE );

        const u32 nb_targets = impl->nb_target;
        for( u32 i = 0; i < nb_targets; ++i )
        {
            const TargetData& data = impl->target[i];
            shader::RenderTargetData& shader_data = shader_target[i];

            const RDITextureInfo info = (data._nb_color_textures) ? data._color_textures[0].info : data._depth_texture.info;

            shader_data.size.x = info.width;
            shader_data.size.y = info.height;
            shader_data.size.z = info.depth;
            shader_data.size.w = info.mips;

            shader_data.size_rcp.x = 1.f / info.width;
            shader_data.size_rcp.y = 1.f / info.height;
            shader_data.size_rcp.z = 1.f / info.depth;
            shader_data.size_rcp.w = 1.f / info.mips;
        }

        SYS_STATIC_ASSERT( sizeof( *shader_camera ) == sizeof( *impl->camera ) );

        memcpy( shader_camera, &impl->camera[0], sizeof( shader::CameraViewData ) * impl->nb_camera );
        memcpy( shader_matrices, &impl->instance[0], impl->offset_instance );

        Unmap( cmdq, impl->gpu_resources.matrices );
        Unmap( cmdq, impl->gpu_resources.camera );
        Unmap( cmdq, impl->gpu_resources.target );
    }

    GpuResources::Bind( &impl->gpu_resources, cmdq );
    
    const u32 begin_cmd = 0;
    const u32 count_cmd = impl->nb_draw_cmd;
    const u32 end_cmd = begin_cmd + count_cmd;

    std::sort( impl->draw_cmd + begin_cmd, impl->draw_cmd + end_cmd, std::less< DrawCmdData >() );

    for( u32 icmd = begin_cmd; icmd < end_cmd; ++icmd )
    {
        const DrawCmdData cmd = impl->draw_cmd[icmd];
        const bool cmd_valid = ValidateDrawCmdData( cmd );
        if( !cmd_valid )
        {
            SYS_LOG_ERROR( "Draw command is invalid!" );
            continue;
        }

        const TargetData& target = impl->target[cmd.target_index];
        const CameraData& camera = impl->camera[cmd.camera_index];
        const PipelineData& pipeline = impl->pipelines[cmd.pipeline_index];
        const GeometryData* geometry = (const GeometryData*)(impl->geometry + cmd.geometry_offset);
        const InstanceData* instances = (const InstanceData*)(impl->instance + cmd.instance_offset);

        ChangeRenderTargets( cmdq, (RDITextureRW*)target._color_textures, target._nb_color_textures, target._depth_texture );
        // SetCamera()

        SetShaderPass( cmdq, pipeline.shader );
        BindResources( cmdq, pipeline.resources );
        
        RDIHardwareState hw_state = AcquireHardwareState( dev, &impl->managed_resources, pipeline.hw_state_desc );
        SetHardwareState( cmdq, hw_state );

        SetTopology( cmdq, pipeline.topology );

        // geometry

        // instances

    }
}

LowRenderer* LowRenderer::StartUp( RDIDevice* dev, BXIAllocator* allocator )
{
    u32 mem_size = sizeof( LowRenderer ) + sizeof( LowRenderer::Impl );
    void* memory = BX_MALLOC( allocator, mem_size, sizeof( void* ) );

    LowRenderer* llr = new(memory) LowRenderer();
    llr->impl = new(llr + 1) LowRenderer::Impl();
    llr->impl->allocator = allocator;

    ::StartUp( llr->impl, dev );

    return llr;
}

void LowRenderer::ShutDown( LowRenderer** llr_handle )
{
    if( !llr_handle[0] )
        return;

    LowRenderer* llr = llr_handle[0];
    BXIAllocator* allocator = llr->impl->allocator;

    ::ShutDown( llr->impl );

    BX_DELETE0( allocator, llr_handle[0] );
}

//
LowRenderer::Pipeline::Pipeline( RDIXPipeline const * pip )
{
    _pipeline = pip;
}


LowRenderer::Pipeline& LowRenderer::Pipeline::HwState( const RDIHardwareStateDesc& desc )
{
    _hw_state_desc = desc;
    return *this;
}

LowRenderer::Pipeline& LowRenderer::Pipeline::Resources( RDIXResourceBinding const * resources )
{
    _resources = resources;
    return *this;
}

LowRenderer::Pipeline& LowRenderer::Pipeline::Topology( const RDIETopology::Enum topo )
{
    _topology = topo;
    return *this;
}

// 
LowRenderer::Target::Target() 
    : _nb_color_textures( 0 )
{}

LowRenderer::Target::Target( const RDITextureRW* ctex, u32 nb_ctex, RDITextureDepth dtex /*= RDITextureDepth() */ )
{
    SYS_ASSERT( nb_ctex <= cRDI_MAX_RENDER_TARGETS );
    for( u32 i = 0; i < nb_ctex; ++i )
        _color_textures[i] = ctex[i];
    
    _nb_color_textures = nb_ctex;
    _depth_texture = dtex;
}

LowRenderer::Target::Target( RDITextureDepth dtex )
{
    _depth_texture = dtex;
    _nb_color_textures = 0;
}

LowRenderer::Geometry::Geometry()
{}

shader::VertexStream* LowRenderer::Geometry::AddStream()
{
    return shader::AddStream( &_vertex_layout );
}

LowRenderer::Geometry& LowRenderer::Geometry::Streams( const shader::VertexStream* streams, u32 nb_streams )
{
    SYS_ASSERT( nb_streams <= MAX_VERTEX_STREAMS );
    for( u32 i = 0; i < nb_streams; ++i )
    {
        _vertex_layout.stream[i] = streams[i];
    }
    _vertex_layout.nb_streams = nb_streams;
    return *this;
}

LowRenderer::Geometry& LowRenderer::Geometry::Vertices( RDIBufferRO v, u32 nb_vertices, u32 begin )
{
    _vertices = v;
    _range.begin = begin;
    _range.count = nb_vertices;
    return *this;
}

LowRenderer::Geometry& LowRenderer::Geometry::Indices( RDIBufferRO i, u32 nb_indices, u32 index_stride, u32 begin )
{
    _indices = i;
    _range.begin = begin;
    _range.count = nb_indices;
    _range.stride = index_stride;
    return *this;
}

LowRenderer::Instance::Instance( const void* data, u32 count, u32 stride ) 
    : _data( (u8*)data ), _count( count ), _stride( stride )
{}

LowRenderer::Camera::Camera()
{}

LowRenderer::Camera::Camera( const CameraParams& params, const CameraMatrices& matrices ) 
    : _params( params ), _matrices( matrices )
{}
