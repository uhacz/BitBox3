#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <memory/memory.h>
#include <filesystem/filesystem.h>

#include <window/window_interface.h>
#include <window/window.h>
#include <application/application.h>
#include <foundation/time.h>

int main( int argc, const char** argv )
{
    // --- startup
    BXMemoryStartUp();
    BXIAllocator* default_allocator = BXDefaultAllocator();

    BXIWindow* window_plug = BXIWindow::New( default_allocator );
    BXWindow* window = window_plug->Create( "BitBox", 1600, 900, false, default_allocator );

    BXIApplication* app_plug = CreateApplication( "ride", default_allocator );
    if( app_plug->Startup( argc, argv, window, default_allocator ) )
    {
        HWND hwnd = (HWND)window->GetSystemHandle( window );

        bool ret = 1;

        BXTimeQuery time_query = BXTimeQuery::Begin();
        
        do
        {
            BXTimeQuery::End( &time_query );
            const uint64_t delta_time_us = time_query.duration_US;
            time_query = BXTimeQuery::Begin();

            MSG msg = { 0 };
            while( PeekMessage( &msg, hwnd, 0U, 0U, PM_REMOVE ) != 0 )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }

            BXInput* input = &window->input;
            BXInput::KeyboardState* kbdState = input->kbd.CurrentState();
            for( int i = 0; i < 256; ++i )
            {
                kbdState->keys[i] = ( GetAsyncKeyState( i ) & 0x8000 ) != 0;
            }

            kbdState->keys[BXInput::eKEY_CAPSLOCK] = ( GetKeyState( VK_CAPITAL ) & 0x0001 );

            InputUpdatePad_XInput( &input->pad, 1 );
            BXInput::ComputeMouseDelta( &window->input.mouse );

            ret = app_plug->Update( window, delta_time_us, default_allocator );

            InputSwap( &window->input );
            InputClear( &window->input, true, false, true );

        } while( ret );
    }// --- if

    // --- shutdown
    app_plug->Shutdown( default_allocator );
    BX_DELETE0( default_allocator, app_plug );

    window_plug->Destroy();
    BXIWindow::Free( default_allocator, &window_plug );

    BXMemoryShutDown();
    return 0;
}