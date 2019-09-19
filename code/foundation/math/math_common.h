#pragma once

#include "vmath.h"

quat_t ShortestRotation( const vec3_t& v0, const vec3_t& v1 );

inline vec4_t MakePlane( const vec3_t& normal, const vec3_t& point )
{
    return vec4_t( normal, -dot( normal, point ) );
}
inline vec3_t ProjectPointOnPlane( const vec3_t& point, const vec4_t& plane )
{
    const vec3_t& n = plane.xyz();
    const vec3_t& Q = point;

    const vec3_t Qp = Q - n * (dot( Q, n ) + plane.w);
    return Qp;
}

inline vec3_t ProjectVectorOnPlane( const vec3_t& vec, const vec4_t& plane )
{
    const vec3_t& n = plane.xyz();
    const vec3_t& V = vec;
    const vec3_t W = V - n * dot( V, n );
    return W;
}