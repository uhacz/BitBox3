#pragma once

#include "containers.h"
#include "../memory/memory.h"

namespace c_array
{
    namespace _internal
    {
        template<typename T>
        void grow( c_array_t<T>*& arr, u32 new_capacity, BXIAllocator* allocator )
        {
            c_array_t<T>* new_array = 0;
            if( new_capacity > 0 )
            {
                u32 memory_size = 0;
                memory_size += sizeof( c_array_t<T> );
                memory_size += new_capacity * sizeof( T );
                
                new_array = (c_array_t<T>*)BX_MALLOC( allocator, memory_size, ALIGNOF( T ) );
                new_array->allocator = allocator;
                new_array->capacity = new_capacity;
                new_array->size = 0;

                if( arr && arr->size )
                {
                    const u32 to_copy = (new_capacity < arr->size) ? new_capacity : arr->size;
                    memcpy( new_array->begin(), arr->begin(), sizeof( T )*to_copy );

                    new_array->size = arr->size;
                }
            }

            if( arr )
            {
                BX_FREE0( arr->allocator, arr );
            }
                        
            arr = new_array;
        }
    }//

    template< typename T > T&       front( c_array_t<T>*& arr )       { return arr->data[0]; }
    template< typename T > const T& front( const c_array_t<T>*& arr ) { return arr->data[0]; }
    template< typename T > T&       back ( c_array_t<T>*& arr )       { return arr->data[arr->size - 1]; }
    template< typename T > const T& back ( const c_array_t<T>*& arr ) { return arr->data[arr->size - 1]; }
    template< typename T > void     clear( c_array_t<T>*& arr )       { arr->size = 0; }

    template< typename T >
    void destroy( c_array_t<T>*& arr )
    {
        if( arr )
        {
            BXIAllocator* allocator = arr->allocator;
            BX_FREE0( allocator, arr );
        }
    }

    template< typename T >
    void reserve( c_array_t<T>*& arr, u32 capacity, BXIAllocator* allocator )
    {
        if( !arr || capacity > arr->capacity )
        {
            BXIAllocator* allocator_to_use = (arr) ? arr->allocator : allocator;
            _internal::grow( arr, capacity, allocator_to_use );
        }
    }

    template< typename T >
    void resize( c_array_t<T>*& arr, u32 size, BXIAllocator* allocator )
    {
        if( !arr || size > arr->capacity )
        {
            BXIAllocator* allocator_to_use = (arr) ? arr->allocator : allocator;
            _internal::grow( arr, size, allocator_to_use );
        }

        arr->size = size;
    }

    template< typename T >
    void resize( c_array_t<T>*& arr, u32 size, BXIAllocator* allocator, const T& value )
    {
        const u32 old_size = (arr) ? arr->size : 0;
        resize( arr, size, allocator );

        for( u32 i = old_size; i < size; ++i )
            arr->data[i] = value;
    }

    template< typename T >
    u32 push_back( c_array_t<T>*& arr, const T& value )
    {
        SYS_ASSERT( arr != nullptr );

        if( arr->size + 1 > arr->capacity )
        {
            const u32 new_capacity = (arr->size) ? arr->size * 2 : 8;
            _internal::grow( arr, new_capacity, arr->allocator );
        }

        const u32 index = arr->size++;
        arr->data[index] = value;
        return index;
    }

    template< typename T >
    T pop_back( c_array_t<T>*& arr, const T& default_value = T() )
    {
        if( !arr )
            return default_value;

        arr->size = (arr->size > 0) ? --arr->size : 0;
        return arr->data[arr->size];
    }

    template< typename T >
    void erase( c_array_t<T>*& arr, u32 pos )
    {
        if( !arr || pos > arr->size )
            return;

        for( uint32_t i = pos + 1; i < arr->size; ++i )
        {
            arr->data[i - 1] = arr->data[i];
        }
        pop_back( arr );
    }

    template< typename T >
    void erase_swap( c_array_t<T>*& arr, u32 pos )
    {
        if( !arr || pos > arr->size )
            return;

        if( pos != arr->size - 1 )
        {
            arr->data[pos] = back( arr );
        }
        pop_back( arr );
    }

}//
