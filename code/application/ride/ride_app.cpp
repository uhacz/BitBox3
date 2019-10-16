#include "ride_app.h"
#include "window/window.h"

#include "../rdix/rdix.h"
#include "../rdix/rdix_debug_draw.h"
#include "../rdi_backend/rdi_backend.h"
#include "../util/camera.h"
#include "../util/gfx/gfx_utils.h"

#include "time.h"
#include "util/color.h"


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

static GFXUtils* gfx_utils = nullptr;
static Framebuffer gfx_framebuffer;
static Camera _camera;
static CameraInputContext _camera_input;

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
    }
    
    {
        ComputeMatrices( &_camera.matrices, _camera.params, mat44_t( mat33_t::identity(), vec3_t( 0.f, 1.f, 8.f ) ) );
    }

    
    return true;
}

void RideApplication::Shutdown( BXIAllocator* allocator )
{
    GFXUtils::ShutDown( &gfx_utils, _e.rdidev );
    DestroyRenderTarget( _e.rdidev, &gfx_framebuffer.rtarget );
    ENGLowLevel::Shutdown( &_e );
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
        RDIXDebug::AddAxes( mat44_t( quat_t::rotation( 0.445f, normalize( vec3_t( 1.f, 1.f, 1.f ) ) ), vec4_t( 1.f, 1.f, 1.f, 1.f ) ) );
        RDIXDebug::AddSphere( vec3_t( -1.f, 0.f, 2.f ), 0.5f, RDIXDebugParams( color32_t::GREEN() ).Solid() );
        RDIXDebug::AddSphere( vec3_t( -1.f, 0.f,-2.f ), 0.5f );
        RDIXDebug::AddAABB( vec3_t( -1.f, 0.f, 0.f ), vec3_t( 0.5f ) );
    }

    ClearState( _e.rdicmdq );
    gfx_utils->BindSamplers( _e.rdicmdq );

    BindRenderTarget( _e.rdicmdq, gfx_framebuffer.rtarget );
    ClearRenderTarget( _e.rdicmdq, gfx_framebuffer.rtarget, 0.0f, 0.0f, 0.0f, 1.f, 1.f );
    {
        const mat44_t viewproj = _camera.matrices.proj_api * _camera.matrices.view;

        BindRenderTarget( _e.rdicmdq, gfx_framebuffer.rtarget, { Framebuffer::FINAL }, true );
        RDIXDebug::Flush( _e.rdicmdq, viewproj );
    }

    ChangeToMainFramebuffer( _e.rdicmdq );
    gfx_utils->CopyTexture( _e.rdicmdq, nullptr, Texture( gfx_framebuffer.rtarget, Framebuffer::FINAL), _camera.params.aspect() );

    Swap( _e.rdicmdq, 0 );

    return true;
}
