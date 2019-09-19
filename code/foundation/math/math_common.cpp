#include "math_common.h"
#include "vec3.h"
#include "quat.h"

quat_t ShortestRotation( const vec3_t& v0, const vec3_t& v1 )
{
    const float d = dot( v0, v1 );
    const vec3_t cv = cross( v0, v1 );

    quat_t q = (d > -1.0f) ? quat_t( cv, 1.f + d ) :
        (::fabs( v0.x ) < 0.1f) ? quat_t( 0.f, v0.z, -v0.y, 0.f ) : quat_t( v0.y, -v0.x, 0.f, 0.f );

    return normalize( q );
}
