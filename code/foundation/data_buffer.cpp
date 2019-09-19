#include "data_buffer.h"
#include "debug.h"
#include "array.h"
#include "common.h"
#include <memory\memory.h>

data_buffer_t::~data_buffer_t()
{
    BX_FREE( allocator, data );
}

namespace data_buffer
{
    
    void create( data_buffer_t* buff, uint32_t initial_capacity, BXIAllocator* allocator, uint32_t alignment )
    {
        destroy( buff );

        SYS_ASSERT( initial_capacity > 0 );

        buff->data = (uint8_t*)BX_MALLOC( allocator, initial_capacity, alignment );
        buff->read_offset = 0;
        buff->write_offset = 0;

        buff->alignment = alignment;
        buff->allocator = allocator;
    }

    void create( data_buffer_t* buff, void* begin_addr, uint32_t capacity )
    {
        destroy( buff );

        buff->data = (uint8_t*)begin_addr;
        buff->capacity = capacity;
    }

    void destroy( data_buffer_t* buff )
    {
        BX_FREE( buff->allocator, buff->data );
        buff[0] = {};
    }

    int32_t read( void* ptr, uint32_t element_size, uint32_t nb_elements, data_buffer_t* buff )
    {
        SYS_ASSERT( buff->write_offset >= buff->read_offset );
        const uint32_t bytes_to_read = element_size * nb_elements;
        const uint32_t bytes_left = buff->write_offset - buff->read_offset;
        const uint32_t n = min_of_2( bytes_left, bytes_to_read );
        if( n )
        {
            memcpy( ptr, buff->data + buff->read_offset, n );
            buff->read_offset += n;
        }
        return n;
    }

    int32_t write( data_buffer_t* buff, const void* ptr, uint32_t element_size, uint32_t nb_elements )
    {
        SYS_ASSERT( buff->capacity >= buff->write_offset );

        const uint32_t space_required = element_size * nb_elements;
        const uint32_t space_left = buff->capacity - buff->write_offset;

        if( space_left < space_required )
        {
            if( buff->allocator )
            {
                uint32_t new_capacity = buff->capacity * 2;
                while( (new_capacity - buff->write_offset ) < space_required )
                    new_capacity *= 2;
            
                uint8_t* new_data = (uint8_t*)BX_MALLOC( buff->allocator, new_capacity, buff->alignment );
                memcpy( new_data, buff->data, buff->write_offset );

                BX_FREE( buff->allocator, buff->data );
                buff->data = new_data;
                buff->capacity = new_capacity;
            }
            else
            {
                SYS_ASSERT( false );
                return -1;
            }
        }

        memcpy( buff->end(), ptr, space_required );

        buff->write_offset += space_required;
        return space_required;
    }

    void seek_write( data_buffer_t* buff, uint32_t offset )
    {
        buff->write_offset = min_of_2( offset, buff->capacity );
    }

}//