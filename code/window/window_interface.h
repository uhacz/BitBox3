#pragma once

#include "window_callback.h"

struct BXWindow;
struct BXIAllocator;

struct BXIWindow
{
	virtual BXWindow* Create		( const char* name, unsigned width, unsigned height, bool full_screen, BXIAllocator* allocator ) = 0;
	virtual void	  Destroy		() = 0;
	virtual bool	  AddCallback	( BXWin32WindowCallback callback, void* userData ) = 0;
	virtual void	  RemoveCallback( BXWin32WindowCallback callback ) = 0;
	
	virtual const BXWindow* GetWindow() const = 0;

    static BXIWindow* New( BXIAllocator* allocator );
    static void Free( BXIAllocator* allocator, BXIWindow** w );
};

