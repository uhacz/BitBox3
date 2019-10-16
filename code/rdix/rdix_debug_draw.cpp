#include "rdix_debug_draw.h"
#include "rdix.h"

#include <foundation/array.h>
#include <util/poly_shape/poly_shape.h>
#include <resource_manager/resource_manager.h>
#include <rdi_backend/rdi_backend.h>

#include <atomic>
#include <algorithm>
#include "util/range_splitter.h"

#include "rdix_shader_interop.h"
namespace shader
{
#include <shaders/hlsl/vertex_layout.h>
}//

namespace RDIXDebug
{
struct Vertex
{
    vec3_t pos;
    uint32_t color;
};

struct Cmd
{
    uint32_t data_offset;
    uint32_t data_count; // depends of cmd type (ex. number of instances, or number of vertices)
    union
    {
        uint32_t key;
        struct  
        {
            uint32_t pipeline_index : 8;
            uint32_t rsource_range_index : 4;
            uint32_t rsource_index : 4;
            uint32_t depth : 16;
        };
    };
};
inline bool operator < ( const Cmd& a, const Cmd& b )
{
    return a.key < b.key;
}

enum EDrawRange : uint32_t
{
    DRAW_RANGE_BOX = 0,
    DRAW_RANGE_SHERE,
    DRAW_RANGE_COUNT,
};

enum ERSource : uint32_t
{
    RSOURCE_OBJ = 0,
    RSOURCE_LINES,
    RSOURCE_COUNT,
};

enum EPipeline : uint32_t
{
    PIPELINE_OBJ_DEPTH_WIREFRAME = 0,
    PIPELINE_OBJ_DEPTH_SOLID,

    PIPELINE_OBJ_NDEPTH_WIREFRAME,
    PIPELINE_OBJ_NDEPTH_SOLID,
    
    PIPELINE_LINES_DEPTH,
    PIPELINE_LINES_NDEPTH,

    PIPELINE_COUNT,
};

static inline EPipeline SelectPipeline( const RDIXDebugParams& params, ERSource rsource_type )
{
    static const EPipeline router[RSOURCE_COUNT][2][2] =
    {
        {
            { PIPELINE_OBJ_NDEPTH_WIREFRAME, PIPELINE_OBJ_NDEPTH_SOLID },
            { PIPELINE_OBJ_DEPTH_WIREFRAME, PIPELINE_OBJ_DEPTH_SOLID }
        },
        {
            { PIPELINE_LINES_NDEPTH, PIPELINE_LINES_NDEPTH },
            { PIPELINE_LINES_DEPTH, PIPELINE_LINES_DEPTH }
        }
    };

    return router[rsource_type][params.use_depth][params.is_solid];
}

static constexpr uint32_t INITIAL_INSTANCE_CAPACITY = 128;
static constexpr uint32_t INITIAL_VERTEX_CAPACITY = 512;
static constexpr uint32_t INITIAL_CMD_CAPACITY = 128;
static constexpr uint32_t GPU_LINE_VBUFFER_CAPACITY = 512; // number of vertices, NOT lines
static constexpr uint32_t GPU_MATRIX_BUFFER_CAPACITY = 64;


template<typename T>
struct Array
{
    T* begin() { return data; }
    T* end() { return data + Size(); }

    bool Empty() const { return count == 0; }
    uint32_t Size() const 
    { 
        const uint32_t current_count = count.load();
        return min_of_2( capacity, current_count ); 
    }


    void Init( uint32_t initial_cap, BXIAllocator* allocator )
    {
        if( initial_cap <= capacity )
            return;

        if( data )
        {
            BX_FREE0( allocator, data );
        }

        data = (T*)BX_MALLOC( allocator, initial_cap * sizeof( T ), ALIGNOF( T ) );
        capacity = initial_cap;
        count = 0;
    }

    T& operator[]( uint32_t i )
    {
        SYS_ASSERT( i < count );
        return data[i];
    }
    const T& operator[]( uint32_t i ) const
    {
        SYS_ASSERT( i < count );
        return data[i];
    }
    
    uint32_t Add( uint32_t amount )
    {
        uint32_t index = count.fetch_add( amount, std::memory_order_acquire );
        if( (index + amount) > capacity )
            return UINT32_MAX;

        return index;
    }

    void Clear( BXIAllocator* allocator )
    {
        const int32_t overflow = count - capacity;
        if( overflow > 0 )
        {
            Init( capacity * 2, allocator );
        }

        count = 0;
    }

    void Free( BXIAllocator* allocator )
    {
        BX_FREE0( allocator, data );
        capacity = 0;
        count = 0;
    }
private:
    T* data = nullptr;
    uint32_t capacity = 0;
    std::atomic_uint32_t count = 0;
};

struct Data
{
    BXIAllocator* allocator = nullptr;

    RDIXRenderSource* rsource[RSOURCE_COUNT];
    RDIXPipeline* pipeline[PIPELINE_COUNT];

    RDIConstantBuffer cbuffer_mdata;
    RDIConstantBuffer cbuffer_idata;
    RDIConstantBuffer cbuffer_vdata;
    RDIConstantBuffer cbuffer_draw_range;

    RDIBufferRO       buffer_matrices;
    RDIBufferRO       buffer_vertices;

    RDIBufferRO       buffer_mesh_vertices;
    RDIBufferRO       buffer_mesh_indices;

    RDIXRenderSourceRange obj_draw_range[DRAW_RANGE_COUNT];
    
    Array<mat44_t> instance_buffer;
    Array<Vertex>  vertex_buffer;

    Array<Cmd> cmd_objects;
    Array<Cmd> cmd_lines;
};

struct InstanceData
{
    uint32_t instance_batch_offset = 0;
    uint32_t padding_[3];
};
struct MaterialData
{
    mat44_t view_proj_matrix = mat44_t::identity();
};


namespace
{
    RDIXPipeline* CreatePipeline( RDIDevice* dev, RDIXShaderFile* shader_file, const char* pass_name, BXIAllocator* allocator )
    {
        RDIXPipelineDesc pipeline_desc( shader_file, pass_name );
        return CreatePipeline( dev, pipeline_desc, allocator );
    }
}//

static Data g_data = {};

void StartUp( RDIDevice* dev, BXIAllocator* allocator )
{
    g_data.allocator = allocator;

    { // render source
        poly_shape_t box, sphere;
        poly_shape::createBox( &box, 1, allocator );
        poly_shape::createShpere( &sphere, 6, allocator );
        
        const uint32_t pos_stride = box.n_elem_pos * sizeof( float );
        const uint32_t index_stride = sizeof( *box.indices );

        const uint32_t nb_vertices = box.num_vertices + sphere.num_vertices;
        const uint32_t nb_indices = box.num_indices + sphere.num_indices;
        
        vec3_t* positions = (vec3_t*)BX_MALLOC( allocator, nb_vertices * pos_stride, 4 );
        memcpy( positions, box.positions, box.num_vertices * pos_stride );
        memcpy( positions + box.num_vertices, sphere.positions, sphere.num_vertices * pos_stride );

        uint32_t* indices = (uint32_t*)BX_MALLOC( allocator, nb_indices * index_stride, 4 );
        memcpy( indices, box.indices, box.num_indices * index_stride );
        memcpy( indices + box.num_indices, sphere.indices, sphere.num_indices * index_stride );

        RDIXRenderSourceRange draw_ranges[2] = {};
        draw_ranges[DRAW_RANGE_BOX] = RDIXRenderSourceRange( 0u, box.num_indices );
        draw_ranges[DRAW_RANGE_SHERE] = RDIXRenderSourceRange( box.num_indices, sphere.num_indices, box.num_vertices );

        RDIXRenderSourceDesc desc;
        desc.Count( nb_vertices, nb_indices );
        desc.VertexBuffer( RDIVertexBufferDesc::POS(), positions );
        desc.IndexBuffer( RDIEType::UINT, indices );
        desc.draw_ranges = draw_ranges;
        desc.num_draw_ranges = (uint32_t)sizeof_array( draw_ranges );

        g_data.rsource[RSOURCE_OBJ] = CreateRenderSource( dev, desc, allocator );

        g_data.obj_draw_range[DRAW_RANGE_BOX] = draw_ranges[DRAW_RANGE_BOX];
        g_data.obj_draw_range[DRAW_RANGE_SHERE] = draw_ranges[DRAW_RANGE_SHERE];

        g_data.buffer_mesh_vertices = CreateRawBufferRO( dev, nb_vertices * pos_stride, positions, 0 );
        g_data.buffer_mesh_indices = CreateRawBufferRO( dev, nb_indices * index_stride, indices, 0 );

        BX_FREE0( allocator, indices );
        BX_FREE0( allocator, positions );

        poly_shape::deallocateShape( &sphere );
        poly_shape::deallocateShape( &box );
    }

    { // render source
        RDIXRenderSourceDesc desc;
        desc.Count( GPU_LINE_VBUFFER_CAPACITY );
        desc.VertexBuffer( RDIVertexBufferDesc::POS4().CPUWrite(), nullptr );
        
        g_data.rsource[RSOURCE_LINES] = CreateRenderSource( dev, desc, allocator );
    }

    {// constant buffer
        g_data.cbuffer_mdata      = CreateConstantBuffer( dev, sizeof( MaterialData ) );
        g_data.cbuffer_idata      = CreateConstantBuffer( dev, sizeof( InstanceData ) );
        g_data.cbuffer_vdata      = CreateConstantBuffer( dev, sizeof( shader::VertexLayout ) );
        g_data.cbuffer_draw_range = CreateConstantBuffer( dev, sizeof( shader::DrawRange ) );
        g_data.buffer_matrices    = CreateStructuredBufferRO( dev, GPU_MATRIX_BUFFER_CAPACITY, sizeof( mat44_t ), RDIECpuAccess::WRITE );
        g_data.buffer_vertices    = CreateRawBufferRO( dev, sizeof( Vertex ) * GPU_LINE_VBUFFER_CAPACITY, nullptr, RDIECpuAccess::WRITE );

    }

    {// shader
        RDIXShaderFile* shader_file = LoadShaderFile( "shader/hlsl/bin/debug.shader", allocator );

        g_data.pipeline[PIPELINE_OBJ_DEPTH_WIREFRAME] = CreatePipeline( dev, shader_file, "object_D_W", allocator );
        g_data.pipeline[PIPELINE_OBJ_NDEPTH_WIREFRAME] = CreatePipeline( dev, shader_file, "object_ND_W", allocator );
        g_data.pipeline[PIPELINE_OBJ_DEPTH_SOLID] = CreatePipeline( dev, shader_file, "object_D_S", allocator );
        g_data.pipeline[PIPELINE_OBJ_NDEPTH_SOLID] = CreatePipeline( dev, shader_file, "object_ND_S", allocator );
        g_data.pipeline[PIPELINE_LINES_DEPTH] = CreatePipeline( dev, shader_file, "lines_D", allocator );
        g_data.pipeline[PIPELINE_LINES_NDEPTH] = CreatePipeline( dev, shader_file, "lines_ND", allocator );

        UnloadShaderFile( &shader_file, allocator );

        // pipeline init
        for( uint32_t i = 0; i < PIPELINE_COUNT; ++i )
        {
            RDIXResourceBinding* binding = ResourceBinding( g_data.pipeline[i] );
            SetConstantBuffer( binding, "MaterialData", &g_data.cbuffer_mdata );

            uint32_t index = FindResource( binding, "InstanceData" );
            if( index != UINT32_MAX )
            {
                SetConstantBufferByIndex( binding, index, &g_data.cbuffer_idata );
            }

            index = FindResource( binding, "VertexLayoutCB" );
            if( index != UINT32_MAX )
            {
                SetConstantBufferByIndex( binding, index, &g_data.cbuffer_vdata );
            }

            index = FindResource( binding, "DrawRangeCB" );
            if( index != UINT32_MAX )
            {
                SetConstantBufferByIndex( binding, index, &g_data.cbuffer_draw_range );
            }

            index = FindResource( binding, "g_vertex_data" );
            if( index != UINT32_MAX )
            {
                SetResourceROByIndex( binding, index, &g_data.buffer_vertices );
            }

            index = FindResource( binding, "g_mesh_vertex_data" );
            if( index != UINT32_MAX )
            {
                SetResourceROByIndex( binding, index, &g_data.buffer_mesh_vertices );
            }

            index = FindResource( binding, "g_mesh_index_data" );
            if( index != UINT32_MAX )
            {
                SetResourceROByIndex( binding, index, &g_data.buffer_mesh_indices );
            }

            index = FindResource( binding, "g_matrices" );
            if( index != UINT32_MAX )
            {
                SetResourceROByIndex( binding, index, &g_data.buffer_matrices );
            }
        }

        g_data.instance_buffer.Init( INITIAL_INSTANCE_CAPACITY, allocator );
        g_data.vertex_buffer.Init( INITIAL_VERTEX_CAPACITY, allocator );
        g_data.cmd_objects.Init( INITIAL_CMD_CAPACITY, allocator );
        g_data.cmd_lines.Init( INITIAL_CMD_CAPACITY, allocator );
    }
}
void ShutDown( RDIDevice* dev )
{
    g_data.instance_buffer.Free( g_data.allocator );
    g_data.vertex_buffer.Free( g_data.allocator );
    g_data.cmd_lines.Free( g_data.allocator );
    g_data.cmd_objects.Free( g_data.allocator );

    for( uint32_t i = 0; i < PIPELINE_COUNT; ++i )
    {
        DestroyPipeline( &g_data.pipeline[i] );
    }

    {
        ::Destroy( &g_data.buffer_mesh_indices );
        ::Destroy( &g_data.buffer_mesh_vertices );
        ::Destroy( &g_data.buffer_vertices );
        ::Destroy( &g_data.buffer_matrices );
        ::Destroy( &g_data.cbuffer_vdata );
        ::Destroy( &g_data.cbuffer_idata );
        ::Destroy( &g_data.cbuffer_mdata );
    }

    for( uint32_t i = 0; i < RSOURCE_COUNT; ++i )
    {
        DestroyRenderSource( &g_data.rsource[i] );
    }
}

struct ObjectCmd
{
    Cmd* cmd = nullptr;
    mat44_t* matrix = nullptr;
};
static ObjectCmd AddObject( ERSource rsource, EDrawRange draw_range, const RDIXDebugParams& params )
{
    ObjectCmd result;
    const uint32_t data_index = g_data.instance_buffer.Add( 1 );
    if( data_index != UINT32_MAX )
    {
        const uint32_t cmd_index = g_data.cmd_objects.Add( 1 );
        if( cmd_index != UINT32_MAX )
        {
            result.cmd = &g_data.cmd_objects[cmd_index];
            result.matrix = &g_data.instance_buffer[data_index];

            result.cmd->data_offset = data_index;
            result.cmd->data_count = 1;
            result.cmd->rsource_index = rsource;
            result.cmd->rsource_range_index = draw_range;
            result.cmd->pipeline_index = SelectPipeline( params, rsource );
            result.cmd->depth = 0;
        }
    }
    return result;
}

struct LinesCmd
{
    Cmd* cmd = nullptr;
    Vertex* vertices = nullptr;
};

static LinesCmd AddLineVertices( uint32_t num_vertices, const RDIXDebugParams& params )
{
    LinesCmd result;
    const uint32_t data_index = g_data.vertex_buffer.Add( num_vertices );
    if( data_index != UINT32_MAX )
    {
        const uint32_t cmd_index = g_data.cmd_lines.Add( 1 );
        if( cmd_index != UINT32_MAX )
        {
            result.cmd = &g_data.cmd_lines[cmd_index];
            result.vertices = &g_data.vertex_buffer[data_index];

            result.cmd->data_offset = data_index;
            result.cmd->data_count = num_vertices;
            result.cmd->rsource_index = RSOURCE_LINES;
            result.cmd->rsource_range_index = 0;
            result.cmd->pipeline_index = SelectPipeline( params, RSOURCE_LINES );
            result.cmd->depth = 0;
        }
    }
    return result;
}

void AddAABB( const vec3_t& center, const vec3_t& extents, const RDIXDebugParams& params )
{
    if( params.is_solid )
    {
        ObjectCmd objcmd = AddObject( RSOURCE_OBJ, DRAW_RANGE_BOX, params );
        if( objcmd.matrix )
        {
            objcmd.matrix[0] = append_scale( mat44_t::translation( center ), extents * 2.f * params.scale );
            objcmd.matrix[0].c3.w = TypeReinterpert( params.color ).f;
        }
    }
    else
    {
        const Vertex corners[8] =
        {
            { center + vec3_t( -extents.x, -extents.y, extents.z ), params.color },
            { center + vec3_t( extents.x, -extents.y, extents.z ), params.color },
            { center + vec3_t( extents.x, -extents.y,-extents.z ), params.color },
            { center + vec3_t( -extents.x, -extents.y,-extents.z ), params.color },
            { center + vec3_t( -extents.x,  extents.y, extents.z ), params.color },
            { center + vec3_t( extents.x,  extents.y, extents.z ), params.color },
            { center + vec3_t( extents.x,  extents.y,-extents.z ), params.color },
            { center + vec3_t( -extents.x,  extents.y,-extents.z ), params.color },
        };
        static constexpr uint8_t indices[] =
        {
            0,1,
            1,2,
            2,3,
            3,0,

            4,5,
            5,6,
            6,7,
            7,4,

            0,4,
            1,5,
            2,6,
            3,7
        };
        static constexpr uint32_t num_indices = (uint32_t)(sizeof( indices ) / sizeof( *indices ));

        LinesCmd cmd = AddLineVertices( num_indices, params );
        if( cmd.vertices )
        {
            for( uint32_t i = 0; i < num_indices; ++i )
            {
                const uint32_t v_index = indices[i];
                cmd.vertices[i] = corners[v_index];
            }
        }
    }
}
void AddSphere( const vec3_t& pos, float radius, const RDIXDebugParams& params )
{
    ObjectCmd objcmd = AddObject( RSOURCE_OBJ, DRAW_RANGE_SHERE, params );
    if( objcmd.matrix )
    {
        objcmd.matrix[0] = append_scale( mat44_t::translation( pos ), vec3_t( radius * params.scale ) );
        objcmd.matrix[0].c3.w = TypeReinterpert( params.color ).f;
    }
}

void AddLine( const vec3_t& start, const vec3_t& end, const RDIXDebugParams& params )
{
    LinesCmd cmd = AddLineVertices( 2, params );
    if( cmd.vertices )
    {
        cmd.vertices[0].pos = start;
        cmd.vertices[0].color = params.color;
        cmd.vertices[1].pos = end;
        cmd.vertices[1].color = params.color;
    }
}
void AddAxes( const mat44_t& pose, const RDIXDebugParams& params )
{
    RDIXDebugParams params_copy = params;
   

    const vec3_t p = pose.translation();
    const mat33_t r = pose.upper3x3();

    params_copy.color = 0xFF0000FF;
    AddLine( p, p + r.c0 * params.scale, params_copy );
    
    params_copy.color = 0x00FF00FF;
    AddLine( p, p + r.c1 * params.scale, params_copy );
    
    params_copy.color = 0x0000FFFF;
    AddLine( p, p + r.c2 * params.scale, params_copy );
}

struct VertexLayoutBuilder
{
    
};

void Flush( RDICommandQueue* cmdq, const mat44_t& viewproj )
{
    if( g_data.cmd_lines.Empty() && g_data.cmd_objects.Empty() )
        return;

    MaterialData mdata = {};
    mdata.view_proj_matrix = viewproj;
    UpdateCBuffer( cmdq, g_data.cbuffer_mdata, &mdata );

    InstanceData idata = {};
    UpdateCBuffer( cmdq, g_data.cbuffer_idata, &idata );

    

    {
        shader::VertexLayout vlayout;
        memset( &vlayout, 0x00, sizeof( shader::VertexLayout ) );
        shader::VertexStream* vstream = shader::AddStream( &vlayout );
        shader::AddAttrib( vstream, RDIFormat::Float3() );
        UpdateCBuffer( cmdq, g_data.cbuffer_vdata, &vlayout );

        const uint32_t nb_objects = g_data.cmd_objects.Size();
        RangeSplitter splitter = RangeSplitter::SplitByGrab( nb_objects, GPU_MATRIX_BUFFER_CAPACITY );
        while( splitter.ElementsLeft() )
        {
            const RangeSplitter::Grab grab = splitter.NextGrab();
            Cmd* cmd_begin = g_data.cmd_objects.begin() + grab.begin;
            Cmd* cmd_end = cmd_begin + grab.count;
            std::sort( cmd_begin, cmd_end, std::less<Cmd>() );

            mat44_t* mapped_data = (mat44_t*)Map( cmdq, g_data.buffer_matrices, 0, RDIEMapType::WRITE );

            mat44_t* matrix = mapped_data;
            for( Cmd* cmd = cmd_begin; cmd != cmd_end; ++cmd )
            {
                matrix[0] = g_data.instance_buffer[cmd->data_offset];
                ++matrix;
            }
            
            Unmap( cmdq, g_data.buffer_matrices );

            for( uint32_t i = 0; i < grab.count; ++i )
            {
                const Cmd& cmd = cmd_begin[i];
                idata.instance_batch_offset = i;
                UpdateCBuffer( cmdq, g_data.cbuffer_idata, &idata );

                const RDIXRenderSourceRange range = g_data.obj_draw_range[cmd.rsource_range_index];
                shader::DrawRange draw_range;
                draw_range.begin = range.begin;
                draw_range.count = range.count;
                draw_range.base_vertex = range.base_vertex;
                draw_range.stride = sizeof( uint32_t );
                UpdateCBuffer( cmdq, g_data.cbuffer_draw_range, &draw_range );

                RDIXPipeline* pipeline = g_data.pipeline[cmd.pipeline_index];
                RDIXRenderSource* rsource = g_data.rsource[cmd.rsource_index];

                BindPipeline( cmdq, pipeline, true );
                SetTopology( cmdq, RDIETopology::TRIANGLES );
                DrawInstanced( cmdq, draw_range.count, 0, 1 );
                //BindRenderSource( cmdq, rsource );
                //SubmitRenderSourceInstanced( cmdq, rsource, 1, cmd.rsource_range_index );
            }
        }
    }
    
    if( !g_data.cmd_lines.Empty() )
    {
        shader::VertexLayout vlayout;
        memset( &vlayout, 0x00, sizeof( shader::VertexLayout ) );
        shader::VertexStream* vstream = shader::AddStream( &vlayout );
        shader::AddAttrib( vstream, RDIFormat::Float3() );
        shader::AddAttrib( vstream, RDIFormat::Uint() );
        UpdateCBuffer( cmdq, g_data.cbuffer_vdata, &vlayout );

        std::sort( g_data.cmd_lines.begin(), g_data.cmd_lines.end(), std::less<Cmd>() );
        const u32 nb_cmd = g_data.cmd_lines.Size();

        uint32_t current_pipeline_index = g_data.cmd_lines[0].pipeline_index;
        BindPipeline( cmdq, g_data.pipeline[current_pipeline_index], true );
        SetTopology( cmdq, RDIETopology::LINES );
        Vertex* mapped_data = (Vertex*)Map( cmdq, g_data.buffer_vertices, RDIEMapType::WRITE );

        u32 nb_vertices = 0;
        for( u32 icmd = 0; icmd < nb_cmd; ++icmd )
        {
            const Cmd& cmd = g_data.cmd_lines[icmd];
            const Vertex* cmd_data = &g_data.vertex_buffer[cmd.data_offset];

            const bool vertex_overflow = (nb_vertices + cmd.data_count > GPU_LINE_VBUFFER_CAPACITY);
            const bool pipeline_mismatch = current_pipeline_index != cmd.pipeline_index;

            if( vertex_overflow || pipeline_mismatch )
            {
                Unmap( cmdq, g_data.buffer_vertices );
                Draw( cmdq, nb_vertices, 0 );

                mapped_data = (Vertex*)Map( cmdq, g_data.buffer_vertices, RDIEMapType::WRITE );
                nb_vertices = 0;

                if( pipeline_mismatch )
                {
                    BindPipeline( cmdq, g_data.pipeline[cmd.pipeline_index], true );
                    SetTopology( cmdq, RDIETopology::LINES );
                    current_pipeline_index = cmd.pipeline_index;
                }
            }

            Vertex* vertices = mapped_data + nb_vertices;
            for( uint32_t i = 0; i < cmd.data_count; ++i )
            {
                vertices[i] = g_data.vertex_buffer[cmd.data_offset + i];
            }
            nb_vertices += cmd.data_count;

        }

        Unmap( cmdq, g_data.buffer_vertices );
        if( nb_vertices )
        {
            Draw( cmdq, nb_vertices, 0 );
        }

        //RDIVertexBuffer dst_vbuffer = VertexBuffer( g_data.rsource[RSOURCE_LINES], 0 );
        //
        //Vertex* mapped_data = (Vertex*)Map( cmdq, dst_vbuffer, 0, GPU_LINE_VBUFFER_CAPACITY, RDIEMapType::WRITE );
        //uint32_t current_pipeline_index = g_data.cmd_lines[0].pipeline_index;
        //RDIXRenderSourceRange draw_range;
        //draw_range.topology = RDIETopology::LINES;
        //draw_range.begin = 0;
        //draw_range.count = 0;
        //draw_range.base_vertex = 0;

        //BindRenderSource( cmdq, g_data.rsource[RSOURCE_LINES] );
        //BindPipeline( cmdq, g_data.pipeline[current_pipeline_index], true );

        //const uint32_t nb_lines = g_data.cmd_lines.Size();
        //for( uint32_t i = 0; i < nb_lines; ++i )
        //{
        //    const Cmd& cmd = g_data.cmd_lines[i];
        //    const Vertex* cmd_data = &g_data.vertex_buffer[cmd.data_offset];

        //    const bool vertex_overflow = (draw_range.count + cmd.data_count > GPU_LINE_VBUFFER_CAPACITY);
        //    const bool pipeline_mismatch = current_pipeline_index != cmd.pipeline_index;

        //    if( vertex_overflow || pipeline_mismatch )
        //    {
        //        Unmap( cmdq, dst_vbuffer );
        //        SubmitRenderSource( cmdq, g_data.rsource[RSOURCE_LINES], draw_range );

        //        mapped_data = (Vertex*)Map( cmdq, dst_vbuffer, 0, GPU_LINE_VBUFFER_CAPACITY, RDIEMapType::WRITE );
        //        draw_range.count = 0;

        //        if( pipeline_mismatch )
        //        {
        //            BindPipeline( cmdq, g_data.pipeline[cmd.pipeline_index], true );
        //            current_pipeline_index = cmd.pipeline_index;
        //        }
        //    }

        //    Vertex* vertices = mapped_data + draw_range.count;
        //    for( uint32_t i = 0; i < cmd.data_count; ++i )
        //    {
        //        vertices[i] = g_data.vertex_buffer[cmd.data_offset + i];
        //    }
        //    draw_range.count += cmd.data_count;
        //}

        //if( draw_range.count )
        //{
        //    Unmap( cmdq, dst_vbuffer );
        //    SubmitRenderSource( cmdq, g_data.rsource[RSOURCE_LINES], draw_range );
        //}

        //RangeSplitter splitter = RangeSplitter::SplitByGrab( nb_lines, GPU_LINE_VBUFFER_CAPACITY/2 );
        //while( splitter.ElementsLeft() )
        //{
        //    const RangeSplitter::Grab grab = splitter.NextGrab();

        //    Cmd* cmd_begin = g_data.cmd_lines.begin() + grab.begin;
        //    Cmd* cmd_end = cmd_begin + grab.count;
        //    std::sort( cmd_begin, cmd_end, std::less<Cmd>() );

        //    RDIXRenderSourceRange buckets[PIPELINE_COUNT];
        //    memset( &buckets[0], 0x00, sizeof( buckets ) );

        //    Vertex* mapped_data = (Vertex*)Map( cmdq, dst_vbuffer, 0, grab.count, RDIEMapType::WRITE );
        //    Vertex* vertex = mapped_data;
        //    for( const Cmd* cmd = cmd_begin; cmd != cmd_end; ++cmd )
        //    {
        //        for( uint32_t i = 0; i < cmd->data_count; ++i )
        //        {
        //            vertex[i] = g_data.vertex_buffer[cmd->data_offset + i];
        //        }
        //        vertex += cmd->data_count;

        //        buckets[cmd->pipeline_index].count += cmd->data_count;
        //    }
        //    Unmap( cmdq, dst_vbuffer );

        //    BindRenderSource( cmdq, g_data.rsource[RSOURCE_LINES] );

        //    uint32_t bucket_begin = 0;
        //    for( uint32_t i = 0; i < PIPELINE_COUNT; ++i )
        //    {
        //        if( buckets[i].count == 0 )
        //            continue;

        //        buckets[i].topology = RDIETopology::LINES;
        //        buckets[i].begin = bucket_begin;
        //        bucket_begin += buckets[i].count;

        //        BindPipeline( cmdq, g_data.pipeline[i], true );
        //        SubmitRenderSource( cmdq, g_data.rsource[RSOURCE_LINES], buckets[i] );
        //    }
        //}
    }

    g_data.cmd_objects.Clear( g_data.allocator );
    g_data.cmd_lines.Clear( g_data.allocator );
    g_data.instance_buffer.Clear( g_data.allocator );
    g_data.vertex_buffer.Clear( g_data.allocator );
}

}//