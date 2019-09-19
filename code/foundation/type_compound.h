#pragma once

#include "type.h"

union float2_t
{
	f32 xy[2];
	struct { f32 x, y; };

	float2_t() {}
	float2_t( f32 vx, f32 vy )
		: x( vx ), y( vy ) {}
};

union float3_t
{
	f32 xyz[3];
	struct { f32 x, y, z; };

	float3_t() {}
	float3_t( f32 vxyz ) : x( vxyz ), y( vxyz ), z( vxyz ) {}
	float3_t( f32 vx, f32 vy, f32 vz ) : x( vx ), y( vy ), z( vz ) {}
};
inline float3_t operator * ( const float3_t& a, float b ) { return float3_t( a.x * b, a.y * b, a.y * b ); }

union float4_t
{
	f32 xyzw[4];
	struct { f32 x, y, z, w; };

	float4_t() {}
	float4_t( f32 vx, f32 vy, f32 vz, f32 vw ) : x( vx ), y( vy ), z( vz ), w( vw ) {}
	float4_t( const float3_t xyz, f32 vw ) : x( xyz.x ), y( xyz.y ), z( xyz.z ), w( vw ) {}
};

union int32x2_t
{
	struct
	{
		i32 x, y;
	};
	i32 xyz[2];

	int32x2_t() : x( 0 ), y( 0 ) {}
	int32x2_t( i32 all ) : x( all ), y( all ) {}
	int32x2_t( i32 a, i32 b ) : x( a ), y( b ) {}
};
union int32x3_t
{
	struct
	{
		i32 x, y, z;
	};
	i32 xyz[3];

	int32x3_t() : x( 0 ), y( 0 ), z( 0 ) {}
	int32x3_t( i32 all ) : x( all ), y( all ), z( all ) {}
	int32x3_t( i32 a, i32 b, i32 c ) : x( a ), y( b ), z( c ) {}
};

using f32x2 = float2_t;
using f32x3 = float3_t;
using f32x4 = float4_t;
