#include "ride_app.h"
#include "window/window.h"

#include "../rdix/rdix.h"
#include "../rdix/rdix_debug_draw.h"
#include "../rdi_backend/rdi_backend.h"
#include "../util/camera.h"
#include "../util/gfx/gfx_utils.h"

#include "time.h"
#include "util/color.h"
#include "low_rendering.h"

#include <string.h>
#include "util/grid.h"
#include "math/mat44.h"
#include "containers.h"

struct Framebuffer
{
    enum
    {
        FINAL = 0,
        BACK,
        SWAP,
    };
    RDIXRenderTarget* rtarget = nullptr;
};

struct Camera
{
    CameraParams params;
    CameraMatrices matrices;
};

static LowRenderer* low_rnd = nullptr;
static GFXUtils* gfx_utils = nullptr;
static Framebuffer gfx_framebuffer;
static Camera _camera;
static CameraInputContext _camera_input;
static RDIXPipeline* _pipeline = nullptr;
static LowRenderer::Geometry _box;
static LowRenderer::Geometry _sphere;
static LowRenderer::GeometryLayout _geo_layout;

struct VertexPN
{
    vec3_t pos;
    vec3_t nrm;
};

void ToGeometry( LowRenderer::Geometry* out_geo, LowRenderer::GeometryLayout* out_layout, RDIDevice* dev, const poly_shape_t& shape, BXIAllocator* allocator )
{
    LowRenderer::Geometry& geo = out_geo[0];

    const uint32_t pos_stride = shape.n_elem_pos * sizeof( float );
    const uint32_t nrm_stride = shape.n_elem_nrm * sizeof( float );
    const uint32_t index_stride = sizeof( *shape.indices );

    const uint32_t nb_vertices = shape.num_vertices;
    const uint32_t nb_indices = shape.num_indices;

    VertexPN* vertices = (VertexPN*)BX_MALLOC( allocator, nb_vertices * sizeof( VertexPN ), 4 );
    for( u32 i = 0; i < nb_vertices; ++i )
    {
        memcpy( vertices[i].pos.xyz, shape.positions + i * shape.n_elem_pos, pos_stride );
        memcpy( vertices[i].nrm.xyz, shape.normals + i * shape.n_elem_nrm, pos_stride );
    }

    uint32_t* indices = (uint32_t*)BX_MALLOC( allocator, nb_indices * index_stride, 4 );
    memcpy( indices, shape.indices, shape.num_indices * index_stride );

    RDIBufferRO mesh_vertices = CreateRawBufferRO( dev, nb_vertices * sizeof(*vertices), vertices, 0 );
    RDIBufferRO mesh_indices = CreateRawBufferRO( dev, nb_indices * sizeof(*indices), indices, 0 );

    geo.Vertices( mesh_vertices, nb_vertices );
    geo.Indices( mesh_indices, nb_indices, index_stride );

    BX_FREE0( allocator, indices );
    BX_FREE0( allocator, vertices );
    if( out_layout )
    {
        shader::VertexStream* stream = out_layout->AddStream();
        shader::AddAttrib( stream, RDIFormat::Float3() );
        shader::AddAttrib( stream, RDIFormat::Float3() );
    }
}

void DestroyGeometry( LowRenderer::Geometry* geo )
{
    ::Destroy( &geo->_vertices );
    ::Destroy( &geo->_indices );
}

bool RideApplication::Startup( int argc, const char** argv, BXWindow* win, BXIAllocator* allocator )
{
    ENGLowLevel::Startup( &_e, argc, argv, win, allocator );
    
    { 
        RDIXRenderTargetDesc rtdesc( 1920, 1080, 1 );
        rtdesc.Texture( Framebuffer::FINAL, RDIFormat::Float4() );
        rtdesc.Texture( Framebuffer::BACK, RDIFormat::Float4() );
        rtdesc.Texture( Framebuffer::SWAP, RDIFormat::Float4() );
        rtdesc.Depth( RDIEType::DEPTH32F );

        gfx_framebuffer.rtarget = CreateRenderTarget( _e.rdidev, rtdesc, allocator );
    
        GFXUtils::StartUp( &gfx_utils, _e.rdidev, allocator );
    
        low_rnd = LowRenderer::StartUp( _e.rdidev, allocator );
    }
    
    {
        ComputeMatrices( &_camera.matrices, _camera.params, mat44_t( mat33_t::identity(), vec3_t( 0.f, 1.f, 8.f ) ) );
    }

    {
        RDIXShaderFile* shader_file = LoadShaderFile( "shader/hlsl/bin/basic.shader", allocator );

        RDIXPipelineDesc desc;
        desc.Shader( shader_file, "main" );
        _pipeline = CreatePipeline( _e.rdidev, desc, allocator );
        UnloadShaderFile( &shader_file, allocator );
    }

    {
        poly_shape_t shape;
        poly_shape::createBox( &shape, 1, allocator );

        ToGeometry( &_box, &_geo_layout, _e.rdidev, shape, allocator );

        poly_shape::deallocateShape( &shape );
    }
    {
        poly_shape_t shape;
        poly_shape::createShpere( &shape, 7, allocator );
        ToGeometry( &_sphere, nullptr, _e.rdidev, shape, allocator );
        poly_shape::deallocateShape( &shape );
    }
    
    return true;
}


void RideApplication::Shutdown( BXIAllocator* allocator )
{
    DestroyGeometry( &_sphere );
    DestroyGeometry( &_box );
    
    DestroyPipeline( &_pipeline );

    LowRenderer::ShutDown( &low_rnd );
    GFXUtils::ShutDown( &gfx_utils, _e.rdidev );
    DestroyRenderTarget( _e.rdidev, &gfx_framebuffer.rtarget );
    ENGLowLevel::Shutdown( &_e );
}

static void CreateInstancesGrid( array_span_t<mat44_t> buffer, u32 width, u32 height, u32 depth, f32 spacing, const mat44_t& base )
{
    const u32 nb_instances = width * height * depth;
    SYS_ASSERT( nb_instances <= buffer.size() );

    const grid_t grid( width, height, depth );
    for( u32 z = 0; z < depth; ++z )
    {
        for( u32 y = 0; y < height; ++y )
        {
            for( u32 x = 0; x < width; ++x )
            {
                const vec3_t offset = vec3_t( f32( x ), f32( y ), f32( z ) ) * spacing;
                const vec3_t position = mul_as_point( base, offset );

                const u32 index = grid.Index( x, y, z );
                buffer[index] = mat44_t::translation( position );
            }
        }
    }
}

bool RideApplication::Update( BXWindow* win, unsigned long long deltaTimeUS, BXIAllocator* allocator )
{
    if( win->input.IsKeyPressedOnce( BXInput::eKEY_ESC ) )
        return false;

    const float delta_time_sec = (float)BXTime::Micro_2_Sec( deltaTimeUS );
    {
        const float sensitivity_in_pix = delta_time_sec;
        BXInput* input = &win->input;
        BXInput::Mouse* input_mouse = &input->mouse;

        const BXInput::MouseState* current_state = input_mouse->CurrentState();

        _camera_input.UpdateInput(
            current_state->lbutton,
            current_state->mbutton,
            current_state->rbutton,
            current_state->dx,
            current_state->dy,
            sensitivity_in_pix,
            delta_time_sec );
    
        const mat44_t new_camera_world = CameraMovementFPP( _camera_input, _camera.matrices.world, delta_time_sec * 10.f );
        ComputeMatrices( &_camera.matrices, _camera.params, new_camera_world );
    }
    {
        RDIXDebug::AddAxes( mat44_t::identity() );
        //RDIXDebug::AddAxes( mat44_t( quat_t::rotation( 0.445f, normalize( vec3_t( 1.f, 1.f, 1.f ) ) ), vec4_t( 1.f, 1.f, 1.f, 1.f ) ) );
        //RDIXDebug::AddSphere( vec3_t( -1.f, 0.f, 2.f ), 0.5f, RDIXDebugParams( color32_t::GREEN() ).Solid() );
        //RDIXDebug::AddSphere( vec3_t( -1.f, 0.f,-2.f ), 0.5f );
        //RDIXDebug::AddAABB( vec3_t( -1.f, 0.f, 0.f ), vec3_t( 0.5f ), RDIXDebugParams( color32_t::RED() ).Solid() );
    }

    ClearState( _e.rdicmdq );
    gfx_utils->BindSamplers( _e.rdicmdq );

    LowRenderer::Target target( gfx_framebuffer.rtarget, LowRenderer::Target::IncludeDepth::YES );
    target._nb_color_textures = 1;
    target.ClearColor( color32_t::BLACK() ).ClearDepth( 1.f );
    
    const LowRenderer::Camera camera( _camera.params, _camera.matrices );
    const LowRenderer::Pipeline pipeline( _pipeline );
    LowRenderer::Pipeline pipeline_wireframe( _pipeline );
    pipeline_wireframe._hw_state_desc.raster.FillMode( RDIEFillMode::WIREFRAME );

    const u32 width = 8;
    const u32 height = 8;
    const u32 depth = 8;
    const u32 nb_instances = width * height * depth;
    mat44_t boxes[nb_instances];
    mat44_t spheres[nb_instances];

    const f32 time_s = BXTime::Micro_2_Sec( _time_us );
    const mat44_t rotx = mat44_t::rotationx( time_s );
    const mat44_t roty = mat44_t::rotationy( time_s );

    CreateInstancesGrid( to_array_span( &boxes[0], nb_instances )  , width, height, depth, 2.f, rotx * mat44_t::translation( vec3_t( -(f32)width, -(f32)height, -(f32)depth ) * 0.5f ) );
    CreateInstancesGrid( to_array_span( &spheres[0], nb_instances ), width, height, depth, 2.f, roty * mat44_t::translation( vec3_t( -(f32)width, -(f32)height, -(f32)depth ) * 0.5f + vec3_t(0.5f) ) );

    const u32 pipeline_index = low_rnd->AddPipeline( pipeline );
    const u32 pipeline_wire_index = low_rnd->AddPipeline( pipeline_wireframe );

    LowRenderer::DrawCmd draw_cmd;
    draw_cmd.instance_index = low_rnd->AddInstance( LowRenderer::Instance::Typed( boxes, nb_instances ) );
    draw_cmd.geometry_index = low_rnd->AddGeometry( _box );
    draw_cmd.geometry_layout_index = low_rnd->AddGeometryLayout( _geo_layout );
    draw_cmd.geometry_layout_nb_streams = 1;
    draw_cmd.pipeline_index = pipeline_wire_index;
    draw_cmd.camera_index = low_rnd->AddCamera( camera );
    draw_cmd.target_index = low_rnd->AddTarget( target );
    draw_cmd.layer = 0;

    low_rnd->AddDrawCmd( draw_cmd, 0.f );
    
    //draw_cmd.pipeline_index = pipeline_wire_index;
    //low_rnd->AddDrawCmd( draw_cmd, 0.f );

    // spheres
    draw_cmd.instance_index = low_rnd->AddInstance( LowRenderer::Instance::Typed( spheres, nb_instances ) );
    draw_cmd.geometry_index = low_rnd->AddGeometry( _sphere );
    draw_cmd.pipeline_index = pipeline_wire_index;
    low_rnd->AddDrawCmd( draw_cmd, 0.f );

    //draw_cmd.pipeline_index = pipeline_wire_index;
    //low_rnd->AddDrawCmd( draw_cmd, 0.f );


    //BindRenderTarget( _e.rdicmdq, gfx_framebuffer.rtarget );
    //ClearRenderTarget( _e.rdicmdq, gfx_framebuffer.rtarget, 0.0f, 0.0f, 0.0f, 1.f, 1.f );

    low_rnd->Flush( _e.rdicmdq, _e.rdidev );

    {
        const mat44_t viewproj = _camera.matrices.proj_api * _camera.matrices.view;

        BindRenderTarget( _e.rdicmdq, gfx_framebuffer.rtarget, { Framebuffer::FINAL }, true );
        RDIXDebug::Flush( _e.rdidev, _e.rdicmdq, viewproj );
    }

    ChangeToMainFramebuffer( _e.rdicmdq );
    gfx_utils->CopyTexture( _e.rdicmdq, nullptr, Texture( gfx_framebuffer.rtarget, Framebuffer::FINAL), _camera.params.aspect() );

    Swap( _e.rdicmdq, 0 );

    _time_us += deltaTimeUS;

    return true;
}
