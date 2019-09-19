#include "window_internal.h"
#include <foundation/debug.h>
#include <memory/memory.h>
#include "foundation/string_util.h"

namespace bx
{
    static void _AdjustWindowSize( HWND hwnd, HWND parent_hwnd, uint32_t width, uint32_t height, bool full_screen )
    {
        int screen_width = GetSystemMetrics( SM_CXSCREEN );
        int screen_height = GetSystemMetrics( SM_CYSCREEN );

        if( !full_screen )
        {
            if( !parent_hwnd )
            {
                DWORD window_style = WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;

                RECT client;
                client.left = 0;
                client.top = 0;
                client.right = width;
                client.bottom = height;

                BOOL ret = AdjustWindowRectEx( &client, window_style, FALSE, 0 );
                SYS_ASSERT( ret );
                const int tmp_width = client.right - client.left;
                const int tmp_height = client.bottom - client.top;
                const int x = 0; //screen_width - tmp_width;
                const int y = screen_height - tmp_height;

                MoveWindow( hwnd, x, y, tmp_width, tmp_height, FALSE );
            }
            else
            {
                DWORD window_style = WS_CHILD;// | WS_BORDER;
                WINDOWINFO win_info;
                BOOL res = GetWindowInfo( parent_hwnd, &win_info );
                SYS_ASSERT( res );

                RECT client;
                client.left = 0;
                client.top = 0;
                client.right = win_info.rcWindow.right;
                client.bottom = win_info.rcWindow.bottom;

                MoveWindow( hwnd, 0, 0, client.right, client.bottom, FALSE );
            }
        }
        else
        {
            MoveWindow( hwnd, 0, 0, screen_width, screen_height, FALSE );

            DEVMODE screen_settings;
            memset( &screen_settings, 0, sizeof( screen_settings ) );

            screen_settings.dmSize = sizeof( screen_settings );
            screen_settings.dmPelsWidth = screen_width;
            screen_settings.dmPelsHeight = screen_height;
            screen_settings.dmBitsPerPel = 32;
            screen_settings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            ChangeDisplaySettings( &screen_settings, CDS_FULLSCREEN );

            ShowCursor( FALSE );
        }

        UpdateWindow( hwnd );
    }

    static uintptr_t _GetSystemHandle( const BXWindow* instance )
    {
        SYS_STATIC_ASSERT( sizeof(uintptr_t) == sizeof( HWND ) );

		const size_t offset = offsetof( bx::Window, _win );

        bx::Window* win = ( bx::Window* )( (uint8_t*)instance - offset );
        return (uintptr_t)win->_hwnd;
    }

    static LRESULT CALLBACK DefaultWindowMessageProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
    
    LRESULT CALLBACK DefaultWindowMessageProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
    {
        Window* win = (Window*)GetWindowLongPtr( hwnd, GWLP_USERDATA );
        if( !win)
        {
            return DefWindowProc( hwnd, msg, wParam, lParam );
        }

		int processed = 0;
		for (uint32_t i = 0; i < win->_num_callbacks; ++i)
			processed |= win->_callbacks[i]( (uintptr_t)hwnd, msg, wParam, lParam, win->_callbacks_user_data[i] );
		
		if (processed)
			return DefWindowProc(hwnd, msg, wParam, lParam);


        static bool lButtonPressed = false;
        BXInput* input = &win->_win.input;
        BXInput::KeyboardState* kbdState = input->kbd.CurrentState();
        BXInput::MouseState* mouseState = input->mouse.CurrentState();

        switch( msg )
        {
        case WM_LBUTTONDOWN:
            {
                mouseState->lbutton = 1;
                break;
            }

        case WM_LBUTTONUP:
            {
                mouseState->lbutton = 0;
                break;
            }
        case WM_MBUTTONDOWN:
            {
                mouseState->mbutton = 1;
                break;
            }

        case WM_RBUTTONUP:
            {
                mouseState->rbutton = 0;
                break;
            }

        case WM_RBUTTONDOWN:
            {
                mouseState->rbutton = 1;
                break;
            }

        case WM_MBUTTONUP:
            {
                mouseState->mbutton = 0;
                break;
            }

        case WM_MOUSEMOVE:
            {
                POINTS pt = MAKEPOINTS( lParam );
                const unsigned short x = pt.x;
                const unsigned short y = pt.y;

                mouseState->x = x;
                mouseState->y = y;

                break;
            }

        case WM_SIZE:
            {
                RECT clientRect;
                GetClientRect( win->_hwnd, &clientRect );

                unsigned int clientWindowWidth = clientRect.right - clientRect.left;
                unsigned int clientWindowHeight = clientRect.bottom - clientRect.top;

                win->_win.width = clientWindowWidth;
                win->_win.height = clientWindowHeight;

                break;
            }
        case WM_CLOSE:
            PostQuitMessage( 0 );
            break;

        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;
        case WM_PARENTNOTIFY:
            PostQuitMessage( 0 );
            break;
        case WM_SYSCOMMAND:
            {
                switch( wParam )
                {
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                    break;
                default:
                    break;
                }
            }
            break;
        }

        return DefWindowProc( hwnd, msg, wParam, lParam );
    }


	BXWindow* Window::Create( const char * name, unsigned width, unsigned height, bool full_screen, BXIAllocator * allocator )
	{
		HWND parent_hwnd = nullptr;

		HINSTANCE hinstance = GetModuleHandle( NULL );
		HDC hdc = 0;
		HWND hwnd = 0;

		{ // create window
			WNDCLASS wndClass;
			wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
			wndClass.lpfnWndProc = DefaultWindowMessageProc;
			wndClass.cbClsExtra = 0;
			wndClass.cbWndExtra = 0;
			wndClass.hInstance = hinstance;
			wndClass.hIcon = 0;
			wndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
			wndClass.hbrBackground = NULL;
			wndClass.lpszMenuName = NULL;
			wndClass.lpszClassName = name;

			if( !RegisterClass( &wndClass ) )
			{
				DWORD err = GetLastError();
				printf( "create_window: Error with RegisterClass!\n" );
				return false;
			}

			if( full_screen && !parent_hwnd )
			{
				hwnd = CreateWindowEx(
					WS_EX_APPWINDOW, name, name, WS_POPUP,
					0, 0,
					10, 10,
					NULL, NULL, hinstance, 0 );
			}
			else
			{
				DWORD style = 0;
				DWORD ex_style = 0;
				if( parent_hwnd )
				{
					style = WS_CHILD | WS_BORDER;
					ex_style = WS_EX_TOPMOST | WS_EX_NOPARENTNOTIFY;
				}
				else
				{
					style = WS_BORDER | WS_THICKFRAME;
				}
				hwnd = CreateWindowEx(
					ex_style, name, name, style,
					10, 10,
					100, 100,
					parent_hwnd, NULL, hinstance, 0 );
			}

			if( hwnd == NULL )
			{
				printf( "create_window: Error while creating window (Err: 0x%x)\n", GetLastError() );
				return 0;
			}

			hdc = GetDC( hwnd );
		}

		_win.GetSystemHandle = _GetSystemHandle;
		strcpy_s( _win.name, BXWindow::NAME_LEN, name );

		_hwnd = hwnd;
		_parent_hwnd = parent_hwnd;
		_hinstance = hinstance;
		_hdc = hdc;

		_win.full_screen = full_screen;

		{ // setup window
			_AdjustWindowSize( hwnd, parent_hwnd, width, height, full_screen );
			ShowWindow( hwnd, SW_SHOW );

			RECT win_rect;
			GetClientRect( hwnd, &win_rect );
			width = win_rect.right - win_rect.left;
			height = win_rect.bottom - win_rect.top;
		}

		_win.width = width;
		_win.height = height;

		SetWindowLongPtr( hwnd, GWLP_USERDATA, (LONG_PTR)this );
		return &_win;
	}
	void Window::Destroy()
	{
		ReleaseDC( _hwnd, _hdc );
		DestroyWindow( _hwnd );
		_hwnd = NULL;
		
		UnregisterClass( _win.name, _hinstance );
	}
	const BXWindow* Window::GetWindow() const
	{
		return &_win;
	}
	bool Window::AddCallback( BXWin32WindowCallback callback, void * userData )
	{
		SYS_ASSERT( callback != nullptr );

		std::lock_guard<std::mutex> lock( _callback_lock );

		if( _num_callbacks >= bx::Window::MAX_MSG_CALLBACKS )
			return false;

		const uint32_t index = _num_callbacks++;
		_callbacks[index] = callback;
		_callbacks_user_data[index] = userData;

		return true;
	}
	void Window::RemoveCallback( BXWin32WindowCallback callback )
	{
		std::lock_guard<std::mutex> lock( _callback_lock );
		for( uintptr_t i = 0; i < _num_callbacks; )
		{
			if( callback == _callbacks[i] )
			{
				const uint32_t last_index = _num_callbacks - 1;
				_callbacks[i]           = _callbacks[last_index];
				_callbacks_user_data[i] = _callbacks_user_data[last_index];
				_num_callbacks          = 1;
			}
			else
			{
				++i;
			}
		}
	}
}//

BXIWindow* BXIWindow::New( BXIAllocator* allocator )
{
    return BX_NEW( allocator, bx::Window );
}

void BXIWindow::Free( BXIAllocator* allocator, BXIWindow** w )
{
    BX_DELETE0( allocator, w[0] );
}

