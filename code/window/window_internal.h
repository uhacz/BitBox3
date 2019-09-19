#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include "window.h"
#include "window_interface.h"
#include "window_callback.h"

#include <mutex>

struct BXIAllocator;
namespace bx
{
	struct Window final : BXIWindow
    {
        enum
        {
            MAX_MSG_CALLBACKS = 4,
        };
		BXWindow  _win;
        HWND      _hwnd;
        HWND      _parent_hwnd;
        HINSTANCE _hinstance;
        HDC       _hdc;

		std::mutex				_callback_lock;
		BXWin32WindowCallback	_callbacks[MAX_MSG_CALLBACKS] = {};
		void*					_callbacks_user_data[MAX_MSG_CALLBACKS] = {};
		uint32_t				_num_callbacks = 0;

		virtual BXWindow* Create( const char* name, unsigned width, unsigned height, bool full_screen, BXIAllocator* allocator ) override;
		virtual void	  Destroy() override;
		virtual bool	  AddCallback( BXWin32WindowCallback callback, void* userData ) override;
		virtual void	  RemoveCallback( BXWin32WindowCallback callback ) override;
		
		virtual const BXWindow* GetWindow() const override;
	};
}//

