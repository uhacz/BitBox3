#include "ride_app.h"
#include "window/window.h"

#include "../rdix/rdix.h"

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

static Framebuffer gfx_framebuffer;

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
    }
    
    
    return true;
}

void RideApplication::Shutdown( BXIAllocator* allocator )
{
    DestroyRenderTarget( _e.rdidev, &gfx_framebuffer.rtarget );
    ENGLowLevel::Shutdown( &_e );
}

bool RideApplication::Update( BXWindow* win, unsigned long long deltaTimeUS, BXIAllocator* allocator )
{
    if( win->input.IsKeyPressedOnce( BXInput::eKEY_ESC ) )
        return false;



    return true;
}
