#pragma once

#include "../common.h"

VEC_FORCE_INLINE float length_sqr( const vec4_t& v )
{
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

VEC_FORCE_INLINE float length( const vec4_t& v )
{
    return ::sqrtf( length_sqr( v ) );
}

VEC_FORCE_INLINE float dot( const vec4_t& a, const vec4_t& b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

VEC_FORCE_INLINE float normalized( vec4_t* v )
{
    const float m = length_sqr( *v );
    v[0] = ( m > 0.0f ) ? *v * recip_sqrt( m ) : vec4_t( 0.f );
    return m;
}

VEC_FORCE_INLINE vec4_t normalize( const vec4_t& v )
{
    const float m = length_sqr( v );
    return ( m > 0.f ) ? v * recip_sqrt( m ) : vec4_t( 0.f );
}

VEC_FORCE_INLINE vec4_t mul_per_elem( const vec4_t& a, const vec4_t& b )
{
    return vec4_t( b.x * a.x, b.y * a.y, b.z * a.z, b.w * a.w );
}

VEC_FORCE_INLINE vec4_t min_per_elem( const vec4_t& a, const vec4_t& b )
{
    return vec4_t( min_of_2( a.x, b.x ), min_of_2( a.y, b.y ), min_of_2( a.z, b.z ), min_of_2( a.w, b.w ) );
}

VEC_FORCE_INLINE float min_elem( const vec4_t& v )
{
    return min_of_2( v.x, min_of_2( v.y, min_of_2( v.z, v.w ) ) );
}

VEC_FORCE_INLINE vec4_t max_per_elem( const vec4_t& a, const vec4_t& b )
{
    return vec4_t( max_of_2( a.x, b.x ), max_of_2( a.y, b.y ), max_of_2( a.z, b.z ), max_of_2( a.w, b.w ) );
}

VEC_FORCE_INLINE float max_elem( const vec4_t& v )
{
    return max_of_2( v.x, max_of_2( v.y, max_of_2( v.z, v.w ) ) );
}

VEC_FORCE_INLINE vec4_t abs_per_elem( const vec4_t& v )
{
    return vec4_t( ::fabsf( v.x ), ::fabsf( v.y ), ::fabsf( v.z ), ::fabsf( v.w ) );
}
VEC_FORCE_INLINE vec4_t recip_per_elem( const vec4_t& v )
{
    return vec4_t( 1.f / v.x, 1.f / v.y, 1.f / v.z, 1.f / v.w );
}
