#pragma once

#include "../foundation/type.h"
#include "../foundation/math/vmath.h"

struct CameraParams
{
    f32 h_aperture = 1.8f;
    f32 v_aperture = 1.f;
    f32 focal_length = 50.f;
    f32 znear = 0.25f;
    f32 zfar = 300.f;
    f32 ortho_width = 10.f;
    f32 ortho_height = 10.f;

    f32 aspect() const;
    f32 fov() const;
};

struct CameraMatrices
{
    mat44_t world;
    mat44_t view;
    mat44_t proj;
    mat44_t proj_api;

    vec3_t eye() const { return world.translation(); }
    vec3_t dir() const { return -world.c2.xyz(); }
};

void    ComputeViewport( int32_t xywh[4], float aspect, int dstWidth, int dstHeight, int srcWidth, int srcHeight );
mat44_t PerspectiveMatrix( float fov, float aspect, float znear, float zfar );
void    ComputeMatrices( CameraMatrices* out, const CameraParams& params, const mat44_t& world );


// --- movement utils
struct CameraInputContext
{
	f32 _left_x = 0.f;
	f32 _left_y = 0.f;
	f32 _right_x = 0.f;
	f32 _right_y = 0.f;
	f32 _up_down = 0.f;

    void ReadInput( int mouseL, int mouseM, int mouseR, int mouseDx, int mouseDy, float mouseSensitivityInPix );

	void UpdateInput( int mouseL, int mouseM, int mouseR, int mouseDx, int mouseDy, float mouseSensitivityInPix, float dt );
	void UpdateInput( float analogX, float analogY, float dt );
	bool AnyMovement() const;
};

mat44_t CameraMovementFPP( const CameraInputContext& input, const mat44_t& world, float sensitivity );
void CameraInputFilter( CameraInputContext* output, const CameraInputContext& raw, const CameraInputContext& prev, f32 rc, f32 dt );
void CameraInputLerp( CameraInputContext* output, f32 t, const CameraInputContext& a, const CameraInputContext& b );