#pragma once

#include <foundation/math/vmath.h>

struct AABB
{
    vec3_t pmin, pmax;

	AABB( const vec3_t& a, const vec3_t& b )
        : pmin(a), pmax(b) {}

	AABB()
        : pmin(-0.5f), pmax(0.5f) {}

    static inline AABB Prepare()
    {
        return AABB( vec3_t( FLT_MAX ), vec3_t( -FLT_MAX ) );
    }

    static inline vec3_t Size( const AABB& bbox )
    {
        return bbox.pmax - bbox.pmin;
    }

    static inline vec3_t Center( const AABB& bbox )
    {
        return bbox.pmin + (bbox.pmax - bbox.pmin ) * 0.5f;
    }

    static inline AABB Extend( const AABB& bbox, const vec3_t& point )
    {
        return AABB( min_per_elem( bbox.pmin, point ), max_per_elem( bbox.pmax, point ) );
    }

    static inline AABB Merge( const AABB& a, const AABB& b )
    {
        return AABB( min_per_elem( a.pmin, b.pmin ), max_per_elem( a.pmax, b.pmax ) );
    }

	static inline AABB Transform( const mat44_t& matrix, const AABB& bbox )
	{
		const vec4_t xb = matrix.c0 * bbox.pmax.x;
		const vec4_t ya = matrix.c1 * bbox.pmin.y;
		const vec4_t yb = matrix.c1 * bbox.pmax.y;
		const vec4_t za = matrix.c2 * bbox.pmin.z;
		const vec4_t zb = matrix.c2 * bbox.pmax.z;
		const vec4_t xa = matrix.c0 * bbox.pmin.x;

		AABB result;
		result.pmin = (min_per_elem( xa, xb ) + min_per_elem( ya, yb ) + min_per_elem( za, zb )).xyz() + matrix.translation();
		result.pmax = (max_per_elem( xa, xb ) + max_per_elem( ya, yb ) + max_per_elem( za, zb )).xyz() + matrix.translation();

		return result;
	}

	//static inline vec_float4 pointInAABBf4( const vec_float4 bboxMin, const vec_float4 bboxMax, const vec_float4 point )
	//{
	//	const vec_float4 a = vec_cmplt( bboxMin, point );
	//	const vec_float4 b = vec_cmpgt( bboxMax, point );
	//	const vec_float4 a_n_b = vec_and( a, b );
	//	return vec_and( vec_splat( a_n_b, 0 ), vec_and( vec_splat( a_n_b, 1 ), vec_splat( a_n_b, 2 ) ) );
	//}

	//static inline bool isPointInside( const bxAABB& aabb, const Vector3& point )
	//{
	//	return boolInVec( floatInVec( pointInAABBf4( aabb.min.get128(), aabb.max.get128(), point.get128() ) ) ).getAsBool();
	//}

	//static inline Vector3 closestPointOnABB( const Vector3& bboxMin, const Vector3& bboxMax, const Vector3& point )
	//{
	//	return minPerElem( maxPerElem( point, bboxMin ), bboxMax );
	//}

};

