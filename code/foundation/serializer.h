#pragma once

#include "data_buffer.h"
#include "blob.h"

#define SRL_ADD( version_added, field )\
    if( srl->version >= version_added )\
    {\
        Serialize( srl, &(field) );\
    }

#define SRL_REM( version_added, version_removed, field )\
    if( srl->version >= version_added && srl->version < version_removed )\
    {\
        Serialize( srl, &(field) );\
    }

struct SRLInstance
{
    uint32_t version;
    
    data_buffer_t data;
    BXIAllocator* allocator;

    uint32_t is_writting;

    static SRLInstance CreateReader( uint32_t v, const void* data, uint32_t data_size, BXIAllocator* allocator );

    static SRLInstance CreateWriterStatic( uint32_t v, const void* data, uint32_t data_size, BXIAllocator* allocator );
    static SRLInstance CreateWriterDynamic( uint32_t v, uint32_t data_size, BXIAllocator* allocator );
};


template<typename T>
inline void Serialize( SRLInstance* srl, T* p )
{
    SYS_STATIC_ASSERT( std::is_trivially_destructible<T>::value );
    SYS_STATIC_ASSERT( !std::is_pointer<T>::value );

    if( srl->is_writting )
    {
        data_buffer::write( &srl->data, p, sizeof( T ), 1 );
    }
    else
    {
        data_buffer::read( p, sizeof( T ), 1, &srl->data );
    }
}

#include <foundation/string_util.h>
template<>
inline void Serialize<string_t>( SRLInstance* srl, string_t* p )
{
    if( srl->is_writting )
    {
        const uint32_t len = string::length( p->c_str() );
        data_buffer::write( &srl->data, &len, sizeof( uint32_t ), 1 );
        data_buffer::write( &srl->data, p->c_str(), len, 1 );
    }
    else
    {
        uint32_t len = 0;
        data_buffer::read( &len, sizeof( uint32_t ), 1, &srl->data );
        string::reserve( p, len, srl->allocator );
        data_buffer::read( p->c_str(), len, 1, &srl->data );
    }
}
