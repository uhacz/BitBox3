#pragma once

#include "containers.h"
#include <memory.h>

namespace array_internal
{
    template< typename T > void _Grow( array_t<T>& arr, int newCapacity )
    {
        T* newData = 0;
        if( newCapacity > 0 )
        {
            newData = (T*)BX_MALLOC( arr.allocator, sizeof(T)*newCapacity, ALIGNOF( T ) );
            const int toCopy = ( (uint32_t)newCapacity < arr.size ) ? (uint32_t)newCapacity : arr.size;
            memcpy( newData, arr.data, sizeof(T)*toCopy );
        }

        BX_FREE0( arr.allocator, arr.data );
        arr.data = newData;
        arr.capacity = newCapacity;
    }
}///

namespace array
{
    template< typename T > T*       begin   ( array_t<T>& arr ) { return arr.data; }
    template< typename T > T*       end     ( array_t<T>& arr ) { return arr.data + arr.size; }
    template< typename T > const T* begin   ( const array_t<T>& arr ) { return arr.data; }
    template< typename T > const T* end     ( const array_t<T>& arr ) { return arr.data + arr.size; }
    template< typename T > int      capacity( const array_t<T>& arr ) { return arr.capacity; }
    template< typename T > uint32_t size    ( const array_t<T>& arr ) { return arr.size; }
    template< typename T > int32_t  sizei   ( const array_t<T>& arr ) { return arr.size; }
    template< typename T > uint32_t size_in_bytes( const array_t<T>& arr ) { return arr.size * sizeof(T); }
    template< typename T > bool     empty   ( const array_t<T>& arr ) { return arr.size == 0; }
    template< typename T > bool     any     ( const array_t<T>& arr ) { return arr.size != 0; }
    template< typename T > T&       front   ( array_t<T>& arr )       { return arr.data[0]; }
    template< typename T > const T& front   ( const array_t<T>& arr ) { return arr.data[0]; }
    template< typename T > T&       back    ( array_t<T>& arr )       { return arr.data[arr.size-1]; }
    template< typename T > const T& back    ( const array_t<T>& arr ) { return arr.data[arr.size-1]; }
    template< typename T > void     clear   ( array_t<T>& arr )       { arr.size = 0; }
    template< typename T > void     destroy ( array_t<T>& arr )       { arr.size = 0; array_internal::_Grow( arr, 0 ); }
}///

namespace array
{
    template< typename T > int push_back( array_t<T>& arr, const T& value )
    {
        if( arr.size + 1 > arr.capacity )
        {
            const uint32_t new_capacity = (arr.capacity) ? arr.capacity * 2 : 8;
            array_internal::_Grow( arr, new_capacity );
        }

        arr.data[arr.size] = value;
        return (int)( arr.size++ );
    }

    template< typename T > void pop_back( array_t<T>& arr )
    {
        arr.size = ( arr.size > 0 ) ? --arr.size : 0;
    }

    template< typename T > void erase_swap( array_t<T>& arr, int pos )
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

    template< typename T > void erase( array_t<T>& arr, int pos )
    {
        const uint32_t upos = (uint32_t)pos;
        if( upos > arr.size )
            return;

        for( uint32_t i = upos + 1; i < arr.size; ++i )
        {
            arr.data[i-1] = arr.data[i];
        }
        pop_back( arr );
    }

    template< typename T > void reserve ( array_t<T>& arr, int newCapacity )
    {
        if( newCapacity > (int)arr.capacity )
            array_internal::_Grow( arr, newCapacity );
    }

    template< typename T > void resize( array_t<T>& arr, int newSize )
    {
        if( newSize > (int)arr.capacity )
            array_internal::_Grow( arr, newSize );

        arr.size = newSize;
    }

    static constexpr uint32_t npos = UINT32_MAX;

    template< typename T > uint32_t find( const array_t<T>& arr, const T& to_find, uint32_t start_index = 0 )
    {
        for( uint32_t i = start_index; i < arr.size; ++i )
        {
            if( to_find == arr[i] )
                return i;
        }

        return npos;
    }

    template< typename T > uint32_t find( const T* begin, uint32_t count, const T& to_find, uint32_t start_index = 0 )
    {
        for( uint32_t i = start_index; i < count; ++i )
        {
            if( to_find == begin[i] )
                return i;
        }

        return npos;
    }
}///

