#pragma once

#include "type.h"


unsigned murmur3_32x86_hash( const void* key, unsigned int size8, unsigned int seed );
void murmur3_128x64_hash( void* out, const void* key, unsigned int size8, unsigned int seed );

__forceinline unsigned murmur3_hash32( const void* key, unsigned int size8, unsigned int seed )
{
    return murmur3_32x86_hash( key, size8, seed );	
}
__forceinline void murmur3_hash128( void* out, const void* key, unsigned size8, unsigned seed )
{
	murmur3_128x64_hash( out, key, size8, seed );
}

unsigned crc32n( const unsigned char* data, unsigned len, unsigned init );
unsigned crc32( const char* data, unsigned init );

