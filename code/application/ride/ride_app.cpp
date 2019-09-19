#include "ride_app.h"
#include "window/window.h"

bool RideApplication::Startup( int argc, const char** argv, BXIAllocator* allocator )
{
    return true;
}

void RideApplication::Shutdown( BXIAllocator* allocator )
{

}

bool RideApplication::Update( BXWindow* win, unsigned long long deltaTimeUS, BXIAllocator* allocator )
{
    if( win->input.IsKeyPressedOnce( BXInput::eKEY_ESC ) )
        return false;

    return true;
}
