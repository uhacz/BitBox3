#include "string_util.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "type.h"
#include "debug.h"
#include "memory/memory.h"
#include "common.h"

char* string::token( char* str, char* tok, size_t toklen, char* delim )
{
    if( !str )
    {
        tok[0] = 0;
        return 0;
    }
    str += strspn( str, delim );
    size_t n = strcspn( str, delim );
    n = ( toklen - 1 < n ) ? toklen - 1 : n;
    memcpy( tok, str, n );
    tok[n] = 0;
    str += n;
    return ( *str == 0 ) ? 0 : str;
}

char* string::duplicate( char* old_string, const char* new_string, BXIAllocator* allocator )
{
    const size_t new_len = strlen( new_string );
    if( old_string )
    {
        const size_t old_len = strlen( old_string );
        if( old_len != new_len )
        {
            BX_FREE0( allocator, old_string );
            old_string = ( char* )BX_MALLOC( allocator, new_len + 1, 1 );
        }
    }
    else
    {
        old_string = (char*)BX_MALLOC( allocator, new_len + 1, 1 );
    }

    strcpy_s( old_string, new_len+1, new_string );
    return old_string;
}

void string::free( char* str, BXIAllocator* allocator )
{
    BX_FREE( allocator, str );
}

char* string::create( const char* str, size_t len, BXIAllocator* allocator )
{
    SYS_ASSERT( strlen( str ) >= len );
    char* out = (char*)BX_MALLOC( allocator, (unsigned)len + 1, 1 ); //memory_alloc( len + 1 );
    out[len] = 0;
    strncpy_s( out, len, str, len );

    return out;
}

unsigned string::count( const char* str, const char c )
{
    unsigned count = 0;
    while( *str )
    {
        const char strc = *str;
        count += strc == c;

        ++str;
    }

    return count;
}
unsigned string::length( const char* str )
{
    return ( str ) ? (unsigned)strlen( str ) : 0;
}
bool string::equal( const char* str0, const char* str1 )
{
    if( !str0 || !str1 )
        return false;

    return strcmp( str0, str1 ) == 0;
}

int string::append( char* buffer, int size_buffer, const char* format, ... )
{
    const size_t N = 1024;
    char str_format[N] = {0};
    
    if( format )
    {
        va_list arglist;
        va_start( arglist, format );
        vsprintf_s( str_format, N, format, arglist );
        va_end( arglist );
    }

    const int len_buff = (int)strlen( buffer );

    if( strlen( str_format ) == 0 )
    {
        return len_buff;
    }

    int len_left = size_buffer - len_buff;
    if( len_left > 0 )
    {
        sprintf_s( buffer + len_buff, len_left, str_format );
        len_left = size_buffer - (int)strlen( buffer );
    }
    return len_left;
}

char* string::find( const char* str, const char* to_find, char** next /*= 0 */ )
{
    char* result = (char*)strstr( str, to_find );
    if( result && next )
    {
        next[0] = (char*)( str + strlen( to_find ) );
    }

    return result;
}

string_t::string_t( const char* str )
{
    SYS_ASSERT( string::length( str ) <= MAX_STATIC_LENGTH );
    string::create( this, str, nullptr );
}


string_t::string_t()
{}

string_t::string_t( string_t&& other )
{
    string::move( this, std::move( other ) );
}

string_t::string_t( const string_t& other )
{
    string::copy( this, other );
}

string_t::~string_t()
{
    string::free( this );
}

string_t& string_t::operator=( const string_t& other )
{
    string::copy( this, other );
    return *this;
}
string_t& string_t::operator=( string_t&& other )
{
    string::move( this, std::move( other ) );
    return *this;
}

unsigned string_t::length() const
{
    return (unsigned)strlen( c_str() );
}

namespace string
{
    void create( string_t* s, const char* data, BXIAllocator* allocator )
    {
        const uint32_t len = length( data );
        if( len > string_t::MAX_STATIC_LENGTH )
        {
            if( !s->_allocator )
            {
                memset( s->_static, 0x00, string_t::MAX_STATIC_SIZE );
            }
            s->_dynamic = string::duplicate( s->_dynamic, data, allocator );
            s->_allocator = allocator;
        }
        else
        {
            if( s->_allocator )
            {
                string::free_and_null( &s->_dynamic, s->_allocator );
            }
            s->_allocator = nullptr;
            strcpy_s( s->_static, data );
        }
    }
    void string::reserve( string_t* s, uint32_t length, BXIAllocator* allocator )
    {
        string::free( s );
        if( length > string_t::MAX_STATIC_LENGTH )
        {
            s->_dynamic = (char*)BX_MALLOC( allocator, length + 1, 1 );
            s->_allocator = allocator;
        }
    }
    void string::copy( string_t* dst, const string_t& src )
    {
        
        if( src._allocator )
        {
            if( !dst->_allocator )
            {
                memset( dst, 0x00, sizeof( string_t ) );
            }

            dst->_allocator = src._allocator;
            dst->_dynamic = string::duplicate( dst->_dynamic, src._dynamic, dst->_allocator );
        }
        else
        {
            memcpy( dst->_static, src._static, string_t::MAX_STATIC_SIZE );
        }
    }
    void string::move( string_t* dst, string_t&& src )
    {
        dst->_allocator = src._allocator;
        if( dst->_allocator )
        {
            dst->_dynamic = src._dynamic;

            src._dynamic = nullptr;
            src._allocator = nullptr;
        }
        else
        {
            memcpy( dst->_static, src._static, string_t::MAX_STATIC_SIZE );
            src._static[0] = 0;
        }
    }



    void free( string_t* s )
    {
        if( s->_allocator )
        {
            string::free_and_null( &s->_dynamic, s->_allocator );
        }
        memset( s, 0x00, sizeof( string_t ) );
    }

    void create( string_buffer_t* s, unsigned capacity, BXIAllocator* allocator )
    {
        free( s );

        s->_base = (char*)BX_MALLOC( allocator, capacity, 1 );
        memset( s->_base, 0x00, capacity );

        s->_offset = 0;
        s->_capacity = capacity;
        s->_allocator = allocator;
    }

    void free( string_buffer_t* s )
    {
        if( !s )
            return;
        
        if( s->_allocator )
        {
            BX_FREE0( s->_allocator, s->_base );
        }
        s->_capacity = 0;
        s->_allocator = nullptr;
    }
    const char* append( string_buffer_t* s, const char* str, char terminator )
    {
        const uint32_t len = string::length( str );
        return appendn( s, str, len, terminator );
    }
    const char* appendn( string_buffer_t* s, const char* str, unsigned len, char terminator )
    {
        const uint32_t space_required = len + 1;
        if( s->_offset + space_required > s->_capacity )
        {
            uint32_t new_capacity = max_of_2( 16u, s->_capacity * 2u );
            while( new_capacity < (s->_offset + space_required ))
            {
                new_capacity *= 2;
            }

            BXIAllocator* allocator = s->_allocator;

            char* new_data = (char*)BX_MALLOC( allocator, new_capacity, 1 );
            memcpy( new_data, s->_base, s->_offset );

            BX_FREE( allocator, s->_base );
            s->_base = new_data;
            s->_capacity = new_capacity;
        }

        char* dst = s->_base + s->_offset;
        memcpy( dst, str, len );
        dst[len] = terminator;

        s->_offset += space_required;
        return dst;
    }

    string_buffer_it string::iterate( const string_buffer_t& s, const string_buffer_it current )
    {
        string_buffer_it it = current;
        if( it.null() )
        {
            it.pointer = s._base;
            it.offset = 0;
        }
        else
        {
            uint32_t search_offset = current.offset;
            while( s._base[++search_offset] )
            {
                if( search_offset >= s._offset )
                {
                    break;
                }
            }

            if( search_offset < s._offset )
            {
                it.offset = search_offset + 1;
                it.pointer = s._base + it.offset;
            }
            else
            {
                it = {};
            }
        }
        return it;
    }
}

string_buffer_t::~string_buffer_t()
{
    string::free( this );
}
