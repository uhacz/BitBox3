#include "mutex.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <memory.h>

#include "../type.h"
#include "../debug.h"


mutex_t::mutex_t( unsigned spin_count )
{
    SYS_STATIC_ASSERT( sizeof( CRITICAL_SECTION ) <= INTERNAL_DATA_SIZE );

    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)&_prv[0];
    bool result = InitializeCriticalSectionAndSpinCount( cs, spin_count );
    SYS_ASSERT( result );
}

mutex_t::~mutex_t()
{
    DeleteCriticalSection( (CRITICAL_SECTION*)&_prv[0] );
}

void mutex_t::lock()
{
    EnterCriticalSection( (CRITICAL_SECTION*)&_prv[0] );
}

void mutex_t::unlock()
{
    LeaveCriticalSection( (CRITICAL_SECTION*)&_prv[0] );
}
