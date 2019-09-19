#pragma once

#include "allocator.h"

#include <new>

#ifndef alignof
#define ALIGNOF(x) __alignof(x)
#endif

#if MEM_USE_DEBUG_ALLOC == 1
    #define BX_MALLOC( a, siz, align ) a->DbgAlloc( a, siz, align, __FILE__, __LINE__, __FUNCTION__ )
    #define BX_ALLOCATE( a, typ ) (typ*)( a->DbgAlloc( a, sizeof(typ), ALIGNOF(typ)), __FILE__, __LINE__, __FUNCTION__ )
    #define BX_FREE( a, ptr ) { if( a ) a->DbgFree( a, ptr ); }
    #define BX_FREE0( a, ptr ) { BX_FREE(a, ptr); ptr = 0; }
    #define BX_NEW(a, T, ...) (new ((a)->DbgAlloc( a, sizeof(T), ALIGNOF(T), __FILE__, __LINE__, __FUNCTION__ )) T(__VA_ARGS__))
#else
#define BX_MALLOC( a, siz, align ) a->Alloc( a, siz, align )
#define BX_ALLOCATE( a, typ ) (typ*)( a->Alloc( a, sizeof(typ), ALIGNOF(typ)) )
#define BX_FREE( a, ptr ) { if( a ) a->Free( a, ptr ); }
#define BX_FREE0( a, ptr ) { BX_FREE(a, ptr); ptr = 0; }
#define BX_NEW(a, T, ...) (new ((a)->Alloc( a, sizeof(T), ALIGNOF(T))) T(__VA_ARGS__))
#endif

template< typename T >
void InvokeDestructor( T* obj )
{
    obj->~T();
}

template<typename T>
void BX_DELETE( BXIAllocator* alloc, T* ptr )
{
    if( ptr )
    {
        ptr->~T();
        BX_FREE( alloc, ptr );
    }
}
#define BX_DELETE0( a, ptr ) { BX_DELETE( a, ptr ); ptr = 0; }

template< typename T, typename... Args >
inline T* BX_RENEW( BXIAllocator* alloc, T** inout, Args&&... args )
{
    if( inout[0] )
        BX_DELETE( alloc, inout[0] );

    inout[0] = BX_NEW( alloc, T, std::forward<Args>( args )... );
    return inout[0];
}

template< typename T >
inline T* BX_REMALLOC( BXIAllocator* alloc, T** buffer, unsigned new_buffer_size, unsigned align = sizeof(void*) )
{
    if( buffer[0] )
        BX_FREE( alloc, buffer[0] );

    buffer[0] = (T*)BX_MALLOC( alloc, new_buffer_size, align );
    return buffer[0];
}

BXIAllocator* BXDefaultAllocator();

void BXMemoryStartUp();
void BXMemoryShutDown();
