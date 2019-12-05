#include "low_rendering.h"

#include "../../rdi_backend/rdi_backend.h"
#include "../../rdix/rdix.h"
#include "atomic"
#include "float16.h"
#include "algorithm"
#include "eastl/hash_map.h"
#include "container_allocator.h"


static constexpr u32 MAX_PIPELINE = 128;
static constexpr u32 MAX_GEOMETRY = 1 << 13;
static constexpr u32 MAX_GEOMETRY_STREAMS = MAX_GEOMETRY * 8;
static constexpr u32 MAX_INSTANCE = 1 << 16;
static constexpr u32 MAX_INSTANCE_DATA = 1 << 16;
static constexpr u32 MAX_CAMERA = 32;
static constexpr u32 MAX_TARGET = 64;
static constexpr u32 MAX_DRAW_CMD = 1 << 17;
static constexpr u32 SIZE_GENERIC_BUFFER = 1024 * 64;

union DrawCmdData
{
    struct
    {
        u64 lo;
        u64 hi;
    } key;
    struct
    {
        u64 depth : 16;
        u64 instance_index : 24;
        u64 geometry_index : 24;

        u64 geometry_layout_nb_streams : 8;
        u64 geometry_layout_first_index : 24;
        u64 pipeline_index : 8;
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
    RDIBufferRO indices;
    shader::DrawRange draw_range;

    u32 streams_index;
    u32 nb_streams;
};

struct InstanceData
{
    shader::float4x4 data;
};
struct InstanceInfo
{
    u32 begin_index;
    u32 count;
};

using CameraData = shader::CameraViewData;
using TargetData = LowRenderer::Target;
using AtomicU32 = std::atomic_uint32_t;



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
    RDIBufferRO vertex_layout;
    RDIConstantBuffer draw_call;

    static void Create( GpuResources* resources, RDIDevice* dev )
    {
        resources->target = CreateStructuredBufferRO( dev, MAX_TARGET, sizeof( shader::RenderTargetData ), RDIECpuAccess::WRITE );
        resources->camera = CreateStructuredBufferRO( dev, MAX_CAMERA, sizeof( shader::CameraViewData ), RDIECpuAccess::WRITE );
        resources->matrices = CreateStructuredBufferRO( dev, MAX_INSTANCE, sizeof( shader::float4x4 ), RDIECpuAccess::WRITE );
        resources->vertex_layout = CreateStructuredBufferRO( dev, MAX_GEOMETRY_STREAMS, sizeof( shader::VertexStream ), RDIECpuAccess::WRITE );
        resources->draw_call = CreateConstantBuffer( dev, sizeof( shader::DrawCallData ) );
    }
    static void Destroy( GpuResources* resources )
    {
        ::Destroy( &resources->draw_call );
        ::Destroy( &resources->vertex_layout );
        ::Destroy( &resources->matrices );
        ::Destroy( &resources->camera );
        ::Destroy( &resources->target );
    }
    static void Bind( GpuResources* resources, RDICommandQueue* cmdq )
    {
        SetResourcesRO( cmdq, &resources->target, 0, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, &resources->camera, 1, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, &resources->matrices, 2, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, &resources->vertex_layout, 3, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetCbuffers   ( cmdq, &resources->draw_call, 16, 1, RDIEPipeline::ALL_STAGES_MASK );
    }
    static void Unbind( GpuResources* resources, RDICommandQueue* cmdq )
    {
        SetResourcesRO( cmdq, nullptr, 0, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, nullptr, 1, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, nullptr, 2, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, nullptr, 3, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetCbuffers   ( cmdq, nullptr, 16, 1, RDIEPipeline::ALL_STAGES_MASK );
    }
};

struct LowRenderer::Impl
{
    ManagedResources managed_resources;
    GpuResources gpu_resources;
    
    u8 generic_data[SIZE_GENERIC_BUFFER];
    PipelineData pipelines[MAX_PIPELINE];
    GeometryData geometry_data[MAX_GEOMETRY];
    shader::VertexStream geometry_stream_desc[MAX_GEOMETRY_STREAMS];
    InstanceData instance_data[MAX_INSTANCE_DATA];
    InstanceInfo instance_info[MAX_INSTANCE];
    CameraData camera[MAX_CAMERA];
    TargetData target[MAX_TARGET];
    DrawCmdData draw_cmd[MAX_DRAW_CMD];

    AtomicU32 offset_generic = 0;
    AtomicU32 nb_pipeline = 0;
    AtomicU32 nb_geometry = 0;
    AtomicU32 nb_geometry_stream_desc = 0;
    AtomicU32 nb_instance_data = 0;
    AtomicU32 nb_instance_info = 0;
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
static void Clear( LowRenderer::Impl* impl )
{
    impl->offset_generic = 0;
    impl->nb_pipeline = 0;
    impl->nb_geometry = 0;
    impl->nb_geometry_stream_desc = 0;
    impl->nb_instance_data = 0;
    impl->nb_instance_info = 0;
    impl->nb_camera = 0;
    impl->nb_target = 0;
    impl->nb_draw_cmd = 0;
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
AllocResult<GeometryData> AllocGeometry( LowRenderer::Impl* impl )
{
    const u32 index = impl->nb_geometry++;
    if( index >= MAX_GEOMETRY )
        return AllocResult<GeometryData>::Null();

    return { &impl->geometry_data[index], index };
}

AllocResult< shader::VertexStream> AllocGeometryStreams( LowRenderer::Impl* impl, u32 nb_streams )
{
    SYS_ASSERT( nb_streams > 0 );
    
    const u32 first_index = impl->nb_geometry_stream_desc.fetch_add( nb_streams );
    const u32 end = first_index + nb_streams;
    if( end > MAX_GEOMETRY_STREAMS )
        return AllocResult< shader::VertexStream>::Null();

    return { &impl->geometry_stream_desc[first_index], first_index };
}

AllocResult<InstanceData> AllocInstanceData( LowRenderer::Impl* impl, u32 nb_instances )
{
    const u32 first_index = impl->nb_instance_data.fetch_add( nb_instances );
    if( first_index + nb_instances >= MAX_INSTANCE_DATA )
        return AllocResult<InstanceData>::Null();

    return { &impl->instance_data[first_index], first_index };
}

AllocResult<InstanceInfo> AllocInstanceInfo( LowRenderer::Impl* impl )
{
    const u32 index = impl->nb_instance_info++;
    if( index >= MAX_INSTANCE )
        return AllocResult<InstanceInfo>::Null();

    return { &impl->instance_info[index], index };
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
    data->view_proj_api = camera._matrices.proj_api * camera._matrices.view;

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
    AllocResult<GeometryData> data = AllocGeometry( impl );
    if( !data )
        return MAX_GEOMETRY;

    data->vertices = geometry._vertices;
    data->indices = geometry._indices;
    data->draw_range = geometry._range;

    return data.index;
}

u32 LowRenderer::AddGeometryLayout( const GeometryLayout& layout )
{
    SYS_ASSERT( layout._layout.nb_streams <= MAX_VERTEX_STREAMS );

    AllocResult<shader::VertexStream> data = AllocGeometryStreams( impl, layout._layout.nb_streams );
    if( !data )
        return MAX_GEOMETRY_STREAMS;

    const u32 n = layout._layout.nb_streams;
    for( u32 i = 0; i < n; ++i )
        data.pointer[i] = layout._layout.stream[i];

    return data.index;
}

u32 LowRenderer::AddInstance( const Instance& instance )
{
    AllocResult<InstanceData> data = AllocInstanceData( impl, instance._count );
    if( !data )
        return MAX_INSTANCE;

    AllocResult<InstanceInfo> info = AllocInstanceInfo( impl );
    if( !info )
        return MAX_INSTANCE;

    info->begin_index = data.index;
    info->count = instance._count;

    SYS_ASSERT( instance._stride == sizeof( InstanceData ) );
    memcpy( data.pointer, instance._data, instance._count * instance._stride );

    return info.index;
}

bool LowRenderer::AddDrawCmd( const DrawCmd& cmd, f32 depth )
{
    AllocResult<DrawCmdData> data = AllocDrawCmd( impl );

    if( !data )
        return false;

    data->key.lo = 0;
    data->key.hi = 0;

    data->depth = float_to_half_fast2( fromF32( depth ) ).u;
    data->instance_index = cmd.instance_index;
    data->geometry_index = cmd.geometry_index;
    
    data->geometry_layout_nb_streams = cmd.geometry_layout_index;
    data->geometry_layout_first_index = cmd.geometry_layout_nb_streams;
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
    result = result && (cmd.geometry_index < MAX_GEOMETRY );
    result = result && (cmd.geometry_layout_first_index + cmd.geometry_layout_nb_streams <= MAX_GEOMETRY_STREAMS);
    result = result && (cmd.instance_index < MAX_INSTANCE );
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
        shader::VertexStream* shader_vertex_desc = (shader::VertexStream*)Map( cmdq, impl->gpu_resources.vertex_layout, RDIEMapType::WRITE );

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
        memcpy( shader_matrices, &impl->instance_data[0], impl->nb_instance_data * sizeof( InstanceData ) );
        memcpy( shader_vertex_desc, &impl->geometry_stream_desc[0], impl->nb_geometry_stream_desc * sizeof( shader::VertexStream ) );

        Unmap( cmdq, impl->gpu_resources.vertex_layout );
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
        const PipelineData& pipeline = impl->pipelines[cmd.pipeline_index];
        const GeometryData& geometry = impl->geometry_data[cmd.geometry_index];
        const InstanceInfo& instance_info = impl->instance_info[cmd.instance_index];

        ChangeRenderTargets( cmdq, (RDITextureRW*)target._color_textures, target._nb_color_textures, target._depth_texture );
        SetShaderPass( cmdq, pipeline.shader );
        BindResources( cmdq, pipeline.resources );
        
        RDIHardwareState hw_state = AcquireHardwareState( dev, &impl->managed_resources, pipeline.hw_state_desc );
        SetHardwareState( cmdq, hw_state );
        SetTopology( cmdq, pipeline.topology );

        shader::DrawCallData draw_call;
        draw_call.target_index = cmd.target_index;
        draw_call.camera_index = cmd.camera_index;
        draw_call.vstream_begin = cmd.geometry_layout_first_index;
        draw_call.vstream_count = cmd.geometry_layout_nb_streams;
        draw_call.first_instance_index = instance_info.begin_index;
        draw_call.draw_range = geometry.draw_range;
        UpdateCBuffer( cmdq, impl->gpu_resources.draw_call, &draw_call );

        SetResourcesRO( cmdq, (RDIBufferRO*)&geometry.vertices, 32, 1, RDIEPipeline::ALL_STAGES_MASK );
        SetResourcesRO( cmdq, (RDIBufferRO*)&geometry.indices, 40, 1, RDIEPipeline::ALL_STAGES_MASK );

        const shader::DrawRange range = geometry.draw_range;

        if( geometry.indices.id )
        {
            DrawIndexedInstanced( cmdq, range.count, range.begin, instance_info.count, range.base_vertex );
        }
        else
        {
            DrawInstanced( cmdq, range.count, range.begin, instance_info.count );
        }
    }

    Clear( impl );
    GpuResources::Unbind( &impl->gpu_resources, cmdq );
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
    InvokeDestructor( llr->impl );

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

shader::VertexStream* LowRenderer::GeometryLayout::AddStream()
{
    return shader::AddStream( &_layout );
}

LowRenderer::GeometryLayout& LowRenderer::GeometryLayout::Streams( const shader::VertexStream* streams, u32 nb_streams )
{
    SYS_ASSERT( nb_streams <= MAX_VERTEX_STREAMS );
    for( u32 i = 0; i < nb_streams; ++i )
    {
        _layout.stream[i] = streams[i];
    }
    _layout.nb_streams = nb_streams;
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
