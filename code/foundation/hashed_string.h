#pragma once

#include "type.h"

using hashed_string_t = uint64_t;

namespace hashed_string_internal
{
    inline hashed_string_t hash_function( const char* str );
}
inline hashed_string_t hashed_string( const char* str )
{
    return hashed_string_internal::hash_function( str );
}


namespace hashed_string_internal
{
    inline hashed_string_t hash_function( const char* str )
    {
        static constexpr uint64_t OFFSET = 14695981039346656037;
        static constexpr uint64_t PRIME = 1099511628211;

        uint64_t hash = OFFSET;
        unsigned char *s = (unsigned char *)str;
        while( *s )
        {
            /* xor the bottom with the current octet */
            hash ^= (uint64_t)*s++;
            /* multiply by the 64 bit FNV magic prime mod 2^64 */
            hash *= PRIME;
        }
        return hash;
    }
}