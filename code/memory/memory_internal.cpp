#include "memory.h"

#include "dlmalloc.h"
#include "allocator.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <mutex>
#include <list>
#include <vector>
#include <algorithm>

namespace bx
{
    struct AllocatorDlmalloc : BXIAllocator
    {
        size_t allocated_size = 0;
    };
    inline void ReportAlloc( BXIAllocator* _this, void* ptr )
    {
        size_t usable_size = dlmalloc_usable_size( ptr );

        AllocatorDlmalloc* alloc = (AllocatorDlmalloc*)_this;
        alloc->allocated_size += usable_size;
    }
    inline void ReportFree( BXIAllocator* _this, void* ptr )
    {
        size_t usable_size = dlmalloc_usable_size( ptr );
        AllocatorDlmalloc* alloc = (AllocatorDlmalloc*)_this;
        assert( alloc->allocated_size >= usable_size );
        alloc->allocated_size -= usable_size;
    }

    static void* DefaultAlloc( BXIAllocator* _this, size_t size, size_t align )
    {
        void* pointer = dlmemalign( align, size );
        ReportAlloc( _this, pointer );
        return pointer;
    }
    static void DefaultFree( BXIAllocator* _this, void* ptr )
    {
        ReportFree( _this, ptr );
        dlfree( ptr );
    }

#if MEM_USE_DEBUG_ALLOC == 1
    struct DebugAllocInfo
    {
        static constexpr unsigned FILE_SIZE = 60;
        static constexpr unsigned FUNC_SIZE = 56;
        char _file[FILE_SIZE];
        char _func[FUNC_SIZE];
        unsigned _line;
        unsigned _size;

        void Set( const char* file, size_t line, const char* func, size_t size )
        {
            strncpy_s( _file, file, FILE_SIZE - 1 );
            strncpy_s( _func, func, FUNC_SIZE - 1 );

            _file[FILE_SIZE - 1] = 0;
            _func[FUNC_SIZE - 1] = 0;

            _line = (unsigned)line;
            _size = (unsigned)size;
        }
    };
    static inline DebugAllocInfo* GetDebugInfo( void* ptr )
    {
        int* offset_ptr = (int*)ptr - 1;
        return (DebugAllocInfo*)((intptr_t)(offset_ptr)+(intptr_t)(*offset_ptr));
    }

    static size_t AlignUp( size_t x, size_t align )
    {
        assert( 0 == (align & (align - 1)) && "must align to a power of two" );
        return (x + (align - 1)) & ~(align - 1);
    }
    
    constexpr size_t DEBUG_INFO_SIZE = sizeof( DebugAllocInfo ) + sizeof( int );

    static std::mutex s_lock;
    static std::vector<DebugAllocInfo*> s_infos;

    static void* DefaultDebugAlloc( BXIAllocator* _this, size_t size, size_t align, const char* file, size_t line, const char* func )
    {
        static_assert(DEBUG_INFO_SIZE == 128, "");
        const size_t requested_size = size;
        const size_t additional_size = AlignUp( DEBUG_INFO_SIZE, align );

        size += additional_size;
        void* pointer = dlmemalign( align, size );
        ReportAlloc( _this, pointer );

        DebugAllocInfo* info = (DebugAllocInfo*)pointer;
        info->Set( file, line, func, requested_size );

        int* offset_ptr = (int*)((char*)info + additional_size - sizeof( int ));
        *offset_ptr = (int)((intptr_t)info - (intptr_t)offset_ptr);

        {
            std::lock_guard<std::mutex> lock( s_lock );
            auto it = std::find( s_infos.begin(), s_infos.end(), info );
            assert( it == s_infos.end() );
            s_infos.push_back( info );
        }

        return (unsigned char*)info + additional_size;

    }
    static void DefaultDebugFree( BXIAllocator* _this, void* ptr )
    {
        if( !ptr )
            return;

        DebugAllocInfo* info = GetDebugInfo( ptr );
        {
            std::lock_guard<std::mutex> lock( s_lock );
            auto it = std::find( s_infos.begin(), s_infos.end(), info );
            assert( it != s_infos.end() );
            s_infos.erase( it );
        }
        
        ReportFree( _this, info );
        dlfree( info );
    }
#endif
    
    static void PrintLeaks()
    {
        perror( "Memory leak(s)!!" );
#if MEM_USE_DEBUG_ALLOC == 1
        for( const DebugAllocInfo* info : s_infos )
        {
            printf( "%u: %s:%d => %s\n", info->_size, info->_file, info->_line, info->_func );
        }
#endif
        system( "PAUSE" );
    }
}

static __declspec(align(64)) bx::AllocatorDlmalloc __default_allocator;

void BXMemoryStartUp()
{
    __default_allocator.Alloc = bx::DefaultAlloc;
    __default_allocator.Free = bx::DefaultFree;
    __default_allocator.DbgAlloc = bx::DefaultDebugAlloc;
    __default_allocator.DbgFree = bx::DefaultDebugFree;
}

void BXMemoryShutDown()
{
    if( __default_allocator.allocated_size != 0 )
    {
        bx::PrintLeaks();           
    }
}

BXIAllocator* BXDefaultAllocator()
{
    return &__default_allocator;
}
