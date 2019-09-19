#pragma once
#include "type.h"

#define BX_UTIL_TAG32( a,b,c,d ) uint32_t( a << 0 | b << 8| c << 16 | d << 24 )
#define BX_UTIL_MAKE_VERSION( major, minor, patch ) uint32_t( ('V' << 24) | (major << 16) | (minor << 8) | (patch) )

union tag32_t
{
	tag32_t() : _tag(0) {}
	explicit tag32_t( const char tag[5] )	{ _tag = *((uint32_t*)tag); }
		
    uint32_t _tag;
	struct
	{
		uint8_t _byte1;
		uint8_t _byte2;
		uint8_t _byte3;
		uint8_t _byte4;
	};
	
	operator const uint32_t() const { return _tag; }
};

union Tag32ToString
{
    Tag32ToString( uint32_t tag )
    : as_u32( tag )
    {
        as_str[4] = 0;
    }
	uint32_t as_u32;
    char as_str[5];
};

union tag64_t
{
	tag64_t() : _lo(0), _hi(0) {}
    explicit tag64_t( const char tag[9] )
        : _lo(0), _hi(0)
    {  
        _i8[0] = tag[0];
        _i8[1] = tag[1];
        _i8[2] = tag[2];
        _i8[3] = tag[3];
        _i8[4] = tag[4];
        _i8[5] = tag[5];
        _i8[6] = tag[6];
        _i8[7] = tag[7];
    }
    
    int8_t   _i8[8];
    uint8_t  _8[8];
    uint16_t _16[4];
    uint32_t _32[2];
    uint64_t _64[1];
    struct
    {
        uint32_t _lo;
        uint32_t _hi;
    };

    operator       uint64_t()       { return _64[0]; }
    operator const uint64_t() const { return _64[0]; }
};

template< char c0, char c1 = 0, char c2 = 0, char c3 = 0, char c4 = 0, char c5 = 0, char c6 = 0, char c7 = 0 >
struct make_tag64
{
    static const uint64_t tag = ( (uint64_t)c0 << 0 ) | ( (uint64_t)c1 << 8 ) | ( (uint64_t)c2 << 16 ) | ( (uint64_t)c3 << 24 ) | ( (uint64_t)c4 << 32 ) | ( (uint64_t)c5 << 40 ) | ( (uint64_t)c6 << 48 ) | ( (uint64_t)c7 << 56 );
};
inline const uint64_t to_tag64( char c0, char c1 = 0, char c2 = 0, char c3 = 0, char c4 = 0, char c5 = 0, char c6 = 0, char c7  = 0 )
{
    return ((uint64_t)c0 << 0) | ((uint64_t)c1 << 8) | ((uint64_t)c2 << 16) | ((uint64_t)c3 << 24) | ((uint64_t)c4 << 32) | ((uint64_t)c5 << 40) | ((uint64_t)c6 << 48) | ((uint64_t)c7 << 56 );
}

inline uint64_t to_Tag64( const char str[] );

union tag128_t
{
	tag128_t() : _lo(0), _hi(0) {}
	explicit tag128_t( const char tag[17] )
		 : _lo(0), _hi(0)
	{  
		uint8_t* src = (uint8_t*)tag;
		uint8_t* dst = _8;
		while( *src )
		{
			*dst = *src;
			++src;
			++dst;
		}
	}
	
	int8_t   _i8[16];
	uint8_t  _8[16];
	uint16_t _16[8];
	uint32_t _32[4];
	uint64_t _64[2];
	struct
	{
		uint64_t _lo;
		uint64_t _hi;
	};

	

};
inline bool operator == ( const tag128_t& a, const tag128_t& b )
{
	return a._lo == b._lo && a._hi == b._hi;
}
inline bool operator != ( const tag128_t& a, const tag128_t& b )
{
	return !( a == b );
}
