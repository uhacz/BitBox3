#pragma once

#include "../common.h"

VEC_FORCE_INLINE quat_t::quat_t( const mat33_t& m )
{
    if( m.c2.z < 0 )
    {
        if( m.c0.x > m.c1.y )
        {
            float t = 1 + m.c0.x - m.c1.y - m.c2.z;
            *this = quat_t( t, m.c0.y + m.c1.x, m.c2.x + m.c0.z, m.c1.z - m.c2.y ) * ( 0.5f / ::sqrtf( t ) );
        }
        else
        {
            float t = 1 - m.c0.x + m.c1.y - m.c2.z;
            *this = quat_t( m.c0.y + m.c1.x, t, m.c1.z + m.c2.y, m.c2.x - m.c0.z ) * ( 0.5f / sqrtf( t ) );
        }
    }
    else
    {
        if( m.c0.x < -m.c1.y )
        {
            float t = 1 - m.c0.x - m.c1.y + m.c2.z;
            *this = quat_t( m.c2.x + m.c0.z, m.c1.z + m.c2.y, t, m.c0.y - m.c1.x ) * ( 0.5f / sqrtf( t ) );
        }
        else
        {
            float t = 1 + m.c0.x + m.c1.y + m.c2.z;
            *this = quat_t( m.c1.z - m.c2.y, m.c2.x - m.c0.z, m.c0.y - m.c1.x, t ) * ( 0.5f / sqrtf( t ) );
        }
    }
}

VEC_FORCE_INLINE quat_t quat_t::rotationx( float radians )
{
	const float a = (radians * 0.5f);
	const float s = ::sinf( a );
	const float c = ::cosf( a );
	return quat_t( s, 0.0f, 0.0f, c );
}

VEC_FORCE_INLINE quat_t quat_t::rotationy( float radians )
{
	const float a = (radians * 0.5f);
	const float s = sinf( a );
	const float c = cosf( a );
	return quat_t( 0.0f, s, 0.0f, c );
}

VEC_FORCE_INLINE quat_t quat_t::rotationz( float radians )
{
	const float a = (radians * 0.5f);
	const float s = sinf( a );
	const float c = cosf( a );
	return quat_t( 0.0f, 0.0f, s, c );
}

VEC_FORCE_INLINE quat_t quat_t::rotation( float radians, const vec3_t& axis )
{
	SYS_ASSERT( ::fabsf( 1.0f - length( axis ) ) < 1e-3f );
	const float a = radians * 0.5f;
	const float s = ::sinf( a );
	const float c = ::cosf( a );
	return quat_t( axis * s, ::cosf( a ) );
}

// returns: xyz - axis, w - angle
VEC_FORCE_INLINE vec4_t to_radians_and_axis( const quat_t& q )
{
    const float epsilon = 1.0e-8f;
    const float s2 = q.x * q.x + q.y * q.y + q.z * q.z;
    if( s2 < epsilon * epsilon ) // can't extract a sensible axis
    {
        return vec4_t( 1.f, 0.f, 0.f, 0.f );
    }
    else
    {
        const float s = recip_sqrt( s2 );
        const vec3_t axis = vec3_t( q.x, q.y, q.z ) * s;
        const float angle = ::fabsf( q.w ) < epsilon ? PI : ::atan2f( s2 * s, q.w ) * 2.0f;
        return vec4_t( axis, angle );
    }
}

VEC_FORCE_INLINE float length_sqr( const quat_t& v )
{
    return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

VEC_FORCE_INLINE float length( const quat_t& v )
{
    return ::sqrtf( length_sqr( v ) );
}

VEC_FORCE_INLINE float dot( const quat_t& a, const quat_t& b )
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

VEC_FORCE_INLINE float normalized( quat_t* v )
{
    const float m = length_sqr( *v );
    v[0] = ( m > 0.0f ) ? *v * recip_sqrt( m ) : quat_t( 0.f );
    return m;
}

VEC_FORCE_INLINE quat_t normalize( const quat_t& v )
{
    const float m = length_sqr( v );
    return ( m > 0.f ) ? v * recip_sqrt( m ) : quat_t( 0.f );
}

VEC_FORCE_INLINE float get_angle( const quat_t& q )
{
    return ::acosf( q.w ) * 2.0f;
}

VEC_FORCE_INLINE float get_angle_between( const quat_t& q1, const quat_t& q2 )
{
    return ::acosf( dot( q1, q2 ) ) * 2.0f;
}

VEC_FORCE_INLINE quat_t conj( const quat_t& q )
{
    return quat_t( -q.x, -q.y, -q.z, q.w );
}

VEC_FORCE_INLINE vec3_t get_axis_x( const quat_t& q )
{
    const float x2 = q.x * 2.0f;
    const float w2 = q.w * 2.0f;
    return vec3_t( ( q.w * w2 ) - 1.0f + q.x * x2, ( q.z * w2 ) + q.y * x2, ( -q.y * w2 ) + q.z * x2 );
}

VEC_FORCE_INLINE vec3_t get_axis_y( const quat_t& q )
{
    const float y2 = q.y * 2.0f;
    const float w2 = q.w * 2.0f;
    return vec3_t( ( -q.z * w2 ) + q.x * y2, ( q.w * w2 ) - 1.0f + q.y * y2, ( q.x * w2 ) + q.z * y2 );
}

VEC_FORCE_INLINE vec3_t get_axis_z( const quat_t& q )
{
    const float z2 = q.z * 2.0f;
    const float w2 = q.w * 2.0f;
    return vec3_t( ( q.y * w2 ) + q.x * z2, ( -q.x * w2 ) + q.y * z2, ( q.w * w2 ) - 1.0f + q.z * z2 );
}

VEC_FORCE_INLINE const vec3_t rotate( const quat_t& q, const vec3_t& v )
{
    const float vx = 2.0f * v.x;
    const float vy = 2.0f * v.y;
    const float vz = 2.0f * v.z;
    const float w2 = q.w * q.w - 0.5f;
    const float dot2 = ( q.x * vx + q.y * vy + q.z * vz );
    return vec3_t( ( vx * w2 + ( q.y * vz - q.z * vy ) * q.w + q.x * dot2 ), 
                   ( vy * w2 + ( q.z * vx - q.x * vz ) * q.w + q.y * dot2 ),
                   ( vz * w2 + ( q.x * vy - q.y * vx ) * q.w + q.z * dot2 ) );
}

VEC_FORCE_INLINE const vec3_t rotate_inv( const quat_t& q, const vec3_t& v )
{
    const float vx = 2.0f * v.x;
    const float vy = 2.0f * v.y;
    const float vz = 2.0f * v.z;
    const float w2 = q.w * q.w - 0.5f;
    const float dot2 = ( q.x * vx + q.y * vy + q.z * vz );
    return vec3_t( ( vx * w2 - ( q.y * vz - q.z * vy ) * q.w + q.x * dot2 ), 
                   ( vy * w2 - ( q.z * vx - q.x * vz ) * q.w + q.y * dot2 ),
                   ( vz * w2 - ( q.x * vy - q.y * vx ) * q.w + q.z * dot2 ) );
}

inline quat_t slerp( float t, const quat_t& left, const quat_t& right )
{
    const float eps = 1.0e-8f;

    float cosine = dot( left, right );
    float sign = 1.f;
    if( cosine < 0 )
    {
        cosine = -cosine;
        sign = -1.f;
    }

    float sine = 1.f - cosine * cosine;

    if( sine >= eps * eps )
    {
        sine = ::sqrtf( sine );
        const float angle = ::atan2f( sine, cosine );
        const float i_sin_angle = 1.f / sine;

        const float leftw = ::sinf( angle * (1.f - t) ) * i_sin_angle;
        const float rightw = ::sinf( angle * t ) * i_sin_angle * sign;

        return left * leftw + right * rightw;
    }

    return left;
}