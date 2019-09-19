#pragma once

#include "containers.h"

struct BXIAllocator;

namespace data_buffer
{
    static constexpr uint32_t DEFAULT_ALIGNMENT = sizeof( void* );

    void create( data_buffer_t* buff, uint32_t initial_capacity, BXIAllocator* allocator, uint32_t alignment = DEFAULT_ALIGNMENT );
    void create( data_buffer_t* buff, void* begin_addr, uint32_t capacity );
    void destroy( data_buffer_t* buff );
   
    int32_t read( void* ptr, uint32_t element_size, uint32_t nb_elements, data_buffer_t* buff );
    int32_t write( data_buffer_t* buff, const void* ptr, uint32_t element_size, uint32_t nb_elements );

    void seek_write( data_buffer_t* buff, uint32_t offset );

    inline uint32_t size( const data_buffer_t& buff ) { return buff.write_offset; }
    

}//
