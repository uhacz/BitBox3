#pragma once

#include "type.h"
#include "debug.h"
#include <math.h>

#define PI 3.14159265358979323846f
#define PI2 6.28318530717958647693f
#define PI_HALF 1.57079632679489661923f
#define PI_INV 0.31830988618379067154f
#define PI_OVER_180 0.01745329251994329577f

inline int bitcount( unsigned int n )
{
    /* works for 32-bit numbers only    */
    /* fix last line for 64-bit numbers */

    unsigned int tmp;
    tmp = n - ( ( n >> 1 ) & 033333333333 ) - ( ( n >> 2 ) & 011111111111 );
    return ( ( tmp + ( tmp >> 3 ) ) & 030707070707 ) % 63;
}

template<typename T>
inline T sqr( const T& t ) { return t * t; }

template<typename Type>
inline Type min_of_2( const Type& a, const Type& b ) {	return ( a < b ) ? a : b; }

template<typename Type>
inline Type max_of_2( const Type& a, const Type& b ) {	return ( a < b ) ? b : a; }

template<typename Type>
inline Type clamp( const Type& x, const Type& a, const Type& b )
{
    return max_of_2( a, min_of_2(b,x) );
}
inline float saturate( float x )
{
    return clamp( x, 0.f, 1.f );
}

inline bool is_equal( f32 f0, f32 f1, const f32 eps = FLT_EPSILON )
{
    const f32 diff = ::fabsf( f0 - f1 );
    f0 = ::fabsf( f0 );
    f1 = ::fabsf( f1 );

    const f32 largest = ( f1 > f0 ) ? f1 : f0;
    return ( diff <= largest * eps ) ? true : false;
}

int32_t  ifloorf( const float x );
inline int32_t iceil( const int32_t x, const int32_t y )
{
    return ( 1 + ( (x-1) / y ) );
}

inline float frac( const float value ) 
{
    return value - float( (int)value );
}

inline float select( float x, float a, float b )
{
    return ( x>=0 ) ? a : b;
}

inline float linearstep( float edge0, float edge1, float x )
{
    return clamp( (x - edge0)/(edge1 - edge0), 0.f, 1.f );
}
inline float smoothstep( float edge0, float edge1, float x )
{
    // Scale, bias and saturate x to 0..1 range
    x = clamp( (x - edge0)/(edge1 - edge0), 0.f, 1.f ); 
    // Evaluate polynomial
    return x*x*(3.f - 2.f*x);
}

inline float smootherstep( float edge0, float edge1, float x )
{
    // Scale, and clamp x to 0..1 range
    x = clamp( (x - edge0)/(edge1 - edge0), 0.f, 1.f );
    // Evaluate polynomial
    return x*x*x*(x*(x*6.f - 15.f) + 10.f);
}

template< typename T >
inline T lerp( float t, const T& a, const T& b )
{
    return a + ( b - a ) * t;
}



inline float cubicpulse( float c, float w, float x )
{
    x = fabsf( x - c );
    if( x > w ) return 0.0f;
    x /= w;
    return 1.0f - x*x*( 3.0f - 2.0f*x );
}

inline unsigned is_pow2( unsigned int x )
{
    return ( ( x != 0 ) && ( ( x & ( ~x + 1 ) ) == x ) );
}

inline float recip_sqrt( float x )
{
    return 1.f / ::sqrtf( x );
}

inline float Sign( float x )
{
    return x < 0.0f ? -1.0f : 1.0f;
}
template <typename T>
inline void Swap( T& a, T& b )
{
    T tmp = a;
    a = b;
    b = tmp;
}

//////////////////////////////////////////////////////////////////////////
#define DECL_WRAP_INC( type_name, type, stype, bit_mask ) \
	static inline type wrap_inc_##type_name( const type val, const type min, const type max ) \
{ \
	__pragma(warning(push))	\
	__pragma(warning(disable:4146))	\
	const type result_inc = val + 1; \
	const type max_diff = max - val; \
	const type max_diff_nz = (type)( (stype)( max_diff | -max_diff ) >> bit_mask ); \
	const type max_diff_eqz = ~max_diff_nz; \
	const type result = ( result_inc & max_diff_nz ) | ( min & max_diff_eqz ); \
	\
	return (result); \
	__pragma(warning(pop))	\
}
DECL_WRAP_INC( uint8_t, uint8_t, int8_t, 7 )
DECL_WRAP_INC( uint16_t, uint16_t, int16_t, 15 )
DECL_WRAP_INC( uint32_t, uint32_t, int32_t, 31 )
DECL_WRAP_INC( uint64_t, uint64_t, int64_t, 63 )
DECL_WRAP_INC( int8_t, int8_t, int8_t, 7 )
DECL_WRAP_INC( int16_t, int16_t, int16_t, 15 )
DECL_WRAP_INC( int32_t, int32_t, int32_t, 31 )
DECL_WRAP_INC( int64_t, int64_t, int64_t, 63 )

///
///
#define DECL_WRAP_DEC( type_name, type, stype, bit_mask ) \
	static inline type wrap_dec_##type_name( const type val, const type min, const type max ) \
{ \
	__pragma(warning(push))	\
	__pragma(warning(disable:4146))	\
	const type result_dec = val - 1; \
	const type min_diff = min - val; \
	const type min_diff_nz = (type)( (stype)( min_diff | -min_diff ) >> bit_mask ); \
	const type min_diff_eqz = ~min_diff_nz; \
	const type result = ( result_dec & min_diff_nz ) | ( max & min_diff_eqz ); \
	\
	return (result); \
	__pragma(warning(pop))	\
} 
DECL_WRAP_DEC( uint8_t, uint8_t, int8_t, 7 )
DECL_WRAP_DEC( uint16_t, uint16_t, int16_t, 15 )
DECL_WRAP_DEC( uint32_t, uint32_t, int32_t, 31 )
DECL_WRAP_DEC( uint64_t, uint64_t, int64_t, 63 )
DECL_WRAP_DEC( int8_t, int8_t, int8_t, 7 )
DECL_WRAP_DEC( int16_t, int16_t, int16_t, 15 )
DECL_WRAP_DEC( int32_t, int32_t, int32_t, 31 )
DECL_WRAP_DEC( int64_t, int64_t, int64_t, 63 )
//////////////////////////////////////////////////////////////////////////

inline uint32_t select( uint32_t a, uint32_t b, uint32_t select_b )
{
	return  ((a ^ b) & select_b) ^ a;
}
