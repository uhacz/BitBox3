#pragma once

#include "containers.h"

namespace array
{
    template< typename T, uint32_t MAX > int      capacity( const static_array_t<T, MAX>& arr ) { return arr.capacity; }
    template< typename T, uint32_t MAX > uint32_t size    ( const static_array_t<T, MAX>& arr ) { return arr.size; }
    template< typename T, uint32_t MAX > bool     empty   ( const static_array_t<T, MAX>& arr ) { return arr.size == 0; }
    template< typename T, uint32_t MAX > bool     any     ( const static_array_t<T, MAX>& arr ) { return arr.size != 0; }
    template< typename T, uint32_t MAX > T&       front   ( static_array_t<T, MAX>& arr ) { return arr.data[0]; }
    template< typename T, uint32_t MAX > const T& front   ( const static_array_t<T, MAX>& arr ) { return arr.data[0]; }
    template< typename T, uint32_t MAX > T&       back    ( static_array_t<T, MAX>& arr ) { return arr.data[arr.size - 1]; }
    template< typename T, uint32_t MAX > const T& back    ( const static_array_t<T, MAX>& arr ) { return arr.data[arr.size - 1]; }
    template< typename T, uint32_t MAX > void     clear   ( static_array_t<T, MAX>& arr ) { arr.size = 0; }

    template< typename T, uint32_t MAX > uint32_t push_back( static_array_t<T, MAX>& arr, const T& value )
    {
        SYS_ASSERT( arr.size + 1 <= MAX );
        arr.data[arr.size] = value;
        return arr.size++;
    }

    template< typename T, uint32_t MAX>
    T& emplace_back( static_array_t<T, MAX>& arr )
    {
        SYS_ASSERT( arr.size + 1 <= MAX );
        return arr.data[arr.size++];
    }

    template< typename T, uint32_t MAX > void pop_back( static_array_t<T, MAX>& arr )
    {
        arr.size = (arr.size > 0) ? --arr.size : 0;
    }

    template< typename T, uint32_t MAX > void erase_swap( static_array_t<T, MAX>& arr, uint32_t pos )
    {
        const uint32_t upos = (uint32_t)pos;
        if( upos > arr.size )
            return;

        if( upos != arr.size - 1 )
        {
            arr.data[upos] = back( arr );
        }
        pop_back( arr );
    }

    template< typename T, uint32_t MAX > void erase( static_array_t<T, MAX>& arr, uint32_t pos )
    {
        const uint32_t upos = (uint32_t)pos;
        if( upos > arr.size )
            return;

        for( uint32_t i = upos + 1; i < arr.size; ++i )
        {
            arr.data[i - 1] = arr.data[i];
        }
        pop_back( arr );
    }

    template< typename T, uint32_t MAX > void resize( static_array_t<T, MAX>& arr, uint32_t newSize )
    {
        SYS_ASSERT( newSize <= MAX );
        arr.size = newSize;
    }
    template< typename T, uint32_t MAX > void resize( static_array_t<T, MAX>& arr, uint32_t newSize, const T& value )
    {
        SYS_ASSERT( newSize <= MAX );
        const u32 begin = arr.size;
        arr.size = newSize;

        for( u32 i = begin; i < arr.size; ++i )
            arr.data[i] = value;

    }
}//
