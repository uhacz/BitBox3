#pragma once

#include "../common.h"

VEC_FORCE_INLINE xform_t::xform_t( const mat44_t& m )
{
    rot = quat_t( mat33_t( m.c0.xyz(), m.c1.xyz(), m.c2.xyz() ) );
    pos = m.c3.xyz();
}

VEC_FORCE_INLINE xform_t inverse( const xform_t& x )
{
    return xform_t( conj( x.rot ), rotate_inv( x.rot, -x.pos ) );
}

VEC_FORCE_INLINE vec3_t xform_point( const xform_t& xform, const vec3_t& point )
{
    return rotate( xform.rot, point ) + xform.pos;
}

VEC_FORCE_INLINE vec3_t xform_inv_point( const xform_t& xform, const vec3_t& point )
{
    return rotate_inv( xform.rot, point - xform.pos );
}

VEC_FORCE_INLINE vec3_t rotate( const xform_t& xform, const vec3_t& vector )
{
    return rotate( xform.rot, vector );
}

VEC_FORCE_INLINE vec3_t rotate_inv( const xform_t& xform, const vec3_t& vector )
{
    return rotate_inv( xform.rot, vector );
}

VEC_FORCE_INLINE xform_t xform_inv( const xform_t& dst, const xform_t& src )
{
    quat_t rotinv = conj( dst.rot );
    return xform_t( rotinv * src.rot, rotate( rotinv, src.pos - dst.pos ) );
}

VEC_FORCE_INLINE xform_t xform_t::operator* ( const xform_t& x ) const 
{ 
    return xform_t( rot * x.rot, rotate( rot, x.pos ) + pos ); 
}