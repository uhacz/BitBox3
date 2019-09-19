#pragma once

#include "../common.h"

VEC_FORCE_INLINE mat33_t::mat33_t( const quat_t& q )
{
    const float x = q.x;
    const float y = q.y;
    const float z = q.z;
    const float w = q.w;

    const float x2 = x + x;
    const float y2 = y + y;
    const float z2 = z + z;

    const float xx = x2 * x;
    const float yy = y2 * y;
    const float zz = z2 * z;

    const float xy = x2 * y;
    const float xz = x2 * z;
    const float xw = x2 * w;

    const float yz = y2 * z;
    const float yw = y2 * w;
    const float zw = z2 * w;

    c0 = vec3_t( 1.0f - yy - zz, xy + zw, xz - yw );
    c1 = vec3_t( xy - zw, 1.0f - xx - zz, yz + xw );
    c2 = vec3_t( xz + yw, yz - xw, 1.0f - xx - yy );
}

VEC_FORCE_INLINE float determinant( const mat33_t& m )
{
    return dot( m.c0, cross( m.c1, m.c2 ) );
}

VEC_FORCE_INLINE mat33_t transpose( const mat33_t& m )
{
    const vec3_t v0( m.c0.x, m.c1.x, m.c2.x );
    const vec3_t v1( m.c0.y, m.c1.y, m.c2.y );
    const vec3_t v2( m.c0.z, m.c1.z, m.c2.z );

    return mat33_t( v0, v1, v2 );
}

VEC_FORCE_INLINE mat33_t inverse( const mat33_t& m )
{
    const float det = determinant( m );
    mat33_t minv;

    if( det != 0 )
    {
        const float det_rcp = 1.0f / det;

        minv.c0.x = det_rcp * ( m.c1.y * m.c2.z - m.c2.y * m.c1.z );
        minv.c0.y = det_rcp *-( m.c0.y * m.c2.z - m.c2.y * m.c0.z );
        minv.c0.z = det_rcp * ( m.c0.y * m.c1.z - m.c0.z * m.c1.y );
                         
        minv.c1.x = det_rcp *-( m.c1.x * m.c2.z - m.c1.z * m.c2.x );
        minv.c1.y = det_rcp * ( m.c0.x * m.c2.z - m.c0.z * m.c2.x );
        minv.c1.z = det_rcp *-( m.c0.x * m.c1.z - m.c0.z * m.c1.x );
                    
        minv.c2.x = det_rcp * ( m.c1.x * m.c2.y - m.c1.y * m.c2.x );
        minv.c2.y = det_rcp *-( m.c0.x * m.c2.y - m.c0.y * m.c2.x );
        minv.c2.z = det_rcp * ( m.c0.x * m.c1.y - m.c1.x * m.c0.y );

        return minv;
    }
    else
    {
        return mat33_t::identity();
    }
}
