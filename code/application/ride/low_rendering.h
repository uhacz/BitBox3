#pragma once

#include "../../foundation/type.h"
#include "../../rdi_backend/rdi_backend_type.h"
#include "../../rdix/rdix_type.h"
#include "../../util/camera.h"

#include "../../rdix/rdix_shader_interop.h"
namespace shader
{
    #include <shaders/hlsl/vertex_layout.h>
    #include <shaders/hlsl/view_data.h>
}//

struct RDIXPipeline;
struct RDIXResourceBinding;
struct RDICommandQueue;
struct RDIDevice;

struct LowRenderer
{
    struct Pipeline
    {
        Pipeline( RDIXPipeline const * pip );
        
        // overrides
        Pipeline& HwState( const RDIHardwareStateDesc& desc );
        Pipeline& Resources( RDIXResourceBinding const * resources );
        Pipeline& Topology( const RDIETopology::Enum topo );

        RDIXPipeline const * _pipeline;
        RDIHardwareStateDesc _hw_state_desc;
        RDIXResourceBinding const * _resources = 0;
        RDIETopology::Enum _topology = RDIETopology::TRIANGLES;
    };

    struct Geometry
    {
        Geometry();           

        Geometry& Vertices( RDIBufferRO v, u32 nb_vertices, u32 begin = 0 );
        Geometry& Indices( RDIBufferRO i, u32 nb_indices, u32 index_stride, u32 begin = 0 );

        shader::DrawRange _range = {};
        RDIBufferRO _vertices;
        RDIBufferRO _indices;
    };

    struct GeometryLayout
    {
        shader::VertexStream* AddStream();
        GeometryLayout& Streams( const shader::VertexStream* streams, u32 nb_streams );

        shader::VertexLayout _layout;
    };

    struct Instance
    {
        Instance( const void* data, u32 count, u32 stride );

        template< typename T >
        static Instance Typed( const T* data, u32 nb_elements ) { return Instance( (u8*)data, nb_elements, sizeof( T ) ); }

        const u8* _data;
        u32 _count;
        u32 _stride;
    };

    struct Camera
    {
        Camera();
        Camera( const CameraParams& params, const CameraMatrices& matrices );

        CameraParams   _params;
        CameraMatrices _matrices;
    };

    struct Target
    {
        Target();
        Target( const RDITextureRW* ctex, u32 nb_ctex, RDITextureDepth dtex = RDITextureDepth() );
        Target( RDITextureDepth dtex );

        RDITextureRW _color_textures[cRDI_MAX_RENDER_TARGETS];
        RDITextureDepth _depth_texture;
        u32 _nb_color_textures;
    };

    struct DrawCmd
    {
        u32 instance_index;
        u32 geometry_index;
        u32 geometry_layout_index;
        u32 geometry_layout_nb_streams;
        u8 pipeline_index;
        u8 camera_index;
        u8 target_index;
        u8 layer;
    };

    u8  AddTarget  ( const Target& view );
    u8  AddCamera  ( const Camera& camera );
    u8  AddPipeline( const Pipeline& pipeline );
    u32 AddGeometry( const Geometry& geometry );
    u32 AddGeometryLayout( const GeometryLayout& layout );
    u32 AddInstance( const Instance& instance );
    bool AddDrawCmd( const DrawCmd& cmd, f32 depth );

    void Flush( RDICommandQueue* cmdq, RDIDevice* dev );

    //
    static LowRenderer* StartUp( RDIDevice* dev, BXIAllocator* allocator );
    static void ShutDown( LowRenderer** llr );

    struct Impl;
    Impl* impl;
};
