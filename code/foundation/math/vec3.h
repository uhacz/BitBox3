#pragma once

#include "../common.h"

VEC_FORCE_INLINE float length_sqr( const vec3_t& v )
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

VEC_FORCE_INLINE float length( const vec3_t& v )
{
    return ::sqrtf( length_sqr(v) );
}

VEC_FORCE_INLINE float dot( const vec3_t& a, const vec3_t& b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

VEC_FORCE_INLINE vec3_t cross( const vec3_t& a, const vec3_t& b )
{
    return vec3_t( a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x );
}

VEC_FORCE_INLINE float normalized( vec3_t* v )
{
    const float m = length_sqr( *v );
    v[0] = ( m > FLT_EPSILON ) ? *v * recip_sqrt( m ) : vec3_t( 0.f );
    return m;
}

VEC_FORCE_INLINE vec3_t normalize( const vec3_t& v, float tolerance = FLT_EPSILON )
{
    const float m = length_sqr( v );
    return ( m >= tolerance) ? v * recip_sqrt( m ) : vec3_t( 0.f );
}

VEC_FORCE_INLINE vec3_t mul_per_elem( const vec3_t& a, const vec3_t& b )
{
    return vec3_t( b.x * a.x, b.y * a.y, b.z * a.z );
}

VEC_FORCE_INLINE vec3_t min_per_elem( const vec3_t& a, const vec3_t& b )
{
    return vec3_t( min_of_2( a.x, b.x ), min_of_2( a.y, b.y ), min_of_2( a.z, b.z ) );
}

VEC_FORCE_INLINE float min_elem( const vec3_t& v )
{
    return min_of_2( v.x, min_of_2( v.y, v.z ) );
}

VEC_FORCE_INLINE vec3_t max_per_elem( const vec3_t& a, const vec3_t& b )
{
    return vec3_t( max_of_2( a.x, b.x ), max_of_2( a.y, b.y ), max_of_2( a.z, b.z ) );
}

VEC_FORCE_INLINE float max_elem( const vec3_t& v )
{
    return max_of_2( v.x, max_of_2( v.y, v.z ) );
}

VEC_FORCE_INLINE vec3_t abs_per_elem( const vec3_t& v )
{
    return vec3_t( ::fabsf( v.x ), ::fabsf( v.y ), ::fabsf( v.z ) );
}
VEC_FORCE_INLINE uint32_t gteq_per_elem( const vec3_t& a, const vec3_t& b )
{
    const uint32_t x_mask = (a.x >= b.x) ? 0x1 : 0;
    const uint32_t y_mask = (a.y >= b.y) ? 0x2 : 0;
    const uint32_t z_mask = (a.z >= b.z) ? 0x4 : 0;
    return x_mask | y_mask | z_mask;
}
