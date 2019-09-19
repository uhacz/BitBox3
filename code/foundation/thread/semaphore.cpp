#include "semaphore.h"


#include "../debug.h"
#include "../type.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

semaphore_t::semaphore_t( int32_t initialCount )
{
	SYS_STATIC_ASSERT(sizeof(_handle) == sizeof(HANDLE));
	SYS_ASSERT(initialCount >= 0);

	_handle = (uintptr_t)CreateSemaphore(NULL, initialCount, MAXLONG, NULL);
}

semaphore_t::~semaphore_t()
{
	CloseHandle( (HANDLE)_handle );
}

void semaphore_t::signal( int32_t count )
{
	ReleaseSemaphore( (HANDLE)_handle, count, NULL );
}

void semaphore_t::wait()
{
	WaitForSingleObject( (HANDLE)_handle, INFINITE );
}

// ---
light_semaphore_t::light_semaphore_t( int initialCount ) 
	: _count( initialCount )
{
	SYS_ASSERT( initialCount >= 0 );
}

bool light_semaphore_t::try_wait()
{
	int oldCount = _count.load( std::memory_order_relaxed );
	return (oldCount > 0 && _count.compare_exchange_strong( oldCount, oldCount - 1, std::memory_order_acquire ));
}

void light_semaphore_t::wait()
{
	if( !try_wait() )
		wait_with_partial_spinning();
}

void light_semaphore_t::signal( int count )
{
	int oldCount = _count.fetch_add( count, std::memory_order_release );
	int toRelease = -oldCount < count ? -oldCount : count;
	if( toRelease > 0 )
	{
		_sema.signal( toRelease );
	}
}

void light_semaphore_t::wait_with_partial_spinning()
{
	int oldCount;
	// Is there a better way to set the initial spin count?
	// If we lower it to 1000, testBenaphore becomes 15x slower on my Core i7-5930K Windows PC,
	// as threads start hitting the kernel semaphore.
	int spin = 10000;
	while( spin-- )
	{
		oldCount = _count.load( std::memory_order_relaxed );
		if( (oldCount > 0) && _count.compare_exchange_strong( oldCount, oldCount - 1, std::memory_order_acquire ) )
			return;
		std::atomic_signal_fence( std::memory_order_acquire );     // Prevent the compiler from collapsing the loop.
	}
	oldCount = _count.fetch_sub( 1, std::memory_order_acquire );
	if( oldCount <= 0 )
	{
		_sema.wait();
	}
}