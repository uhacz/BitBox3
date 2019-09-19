#pragma once

#include "../memory/memory.h"

struct bx_container_allocator
{
    bx_container_allocator( const char* = NULL )
    { }

    bx_container_allocator( const bx_container_allocator& )
    { }

    bx_container_allocator( const bx_container_allocator&, const char* )
    { }

    bx_container_allocator& operator=( const bx_container_allocator& )
    {
        return *this;
    }

    bool operator==( const bx_container_allocator& )
    {
        return true;
    }

    bool operator!=( const bx_container_allocator& )
    {
        return false;
    }

    void* allocate( size_t n, int /*flags*/ = 0 )
    {
        return BX_MALLOC( BXDefaultAllocator(), n, 4 );
    }

    void* allocate( size_t n, size_t alignment, size_t alignmentOffset, int /*flags*/ = 0 )
    {
        return BX_MALLOC( BXDefaultAllocator(), n, alignment );
    }

    void deallocate( void* p, size_t /*n*/ )
    {
        BX_FREE( BXDefaultAllocator(), p );
    }

    const char* get_name() const
    {
        return "bx_container_allocator";
    }

    void set_name( const char* )
    { }
};