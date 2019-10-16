#include "camera.h"

#include "../foundation/math/vmath.h"
#include "signal_filter.h"

float CameraParams::aspect() const
{
    return( ::fabsf( v_aperture ) < 0.0001f ) ? h_aperture : h_aperture / v_aperture;
}

float CameraParams::fov() const
{
    return 2.f * atanf( ( 0.5f * h_aperture ) / ( focal_length * 0.03937f ) );
}

void ComputeViewport( int32_t xywh[4], float aspect, int dstWidth, int dstHeight, int srcWidth, int srcHeight )
{
    const int windowWidth = dstWidth;
    const int windowHeight = dstHeight;

    const float aspectRT = (float)srcWidth / (float)srcHeight;

    int imageWidth;
    int imageHeight;
    int offsetX = 0, offsetY = 0;


    if( aspect > aspectRT )
    {
        imageWidth = windowWidth;
        imageHeight = (int)( windowWidth / aspect + 0.0001f );
        offsetY = windowHeight - imageHeight;
        offsetY = offsetY / 2;
    }
    else
    {
        float aspect_window = (float)windowWidth / (float)windowHeight;
        if( aspect_window <= aspectRT )
        {
            imageWidth = windowWidth;
            imageHeight = (int)( windowWidth / aspectRT + 0.0001f );
            offsetY = windowHeight - imageHeight;
            offsetY = offsetY / 2;
        }
        else
        {
            imageWidth = (int)( windowHeight * aspectRT + 0.0001f );
            imageHeight = windowHeight;
            offsetX = windowWidth - imageWidth;
            offsetX = offsetX / 2;
        }
    }

    xywh[0] = offsetX;
    xywh[1] = offsetY;
    xywh[2] = imageWidth;
    xywh[3] = imageHeight;
}

mat44_t PerspectiveMatrix( float fov, float aspect, float znear, float zfar )
{
    const float f = tanf( ( (float)(PI_HALF)-( 0.5f * fov ) ) );
    const float range_rcp = ( 1.0f / ( znear - zfar ) );
    return mat44_t(
        vec4_t( ( f / aspect ), 0.0f, 0.0f, 0.0f ),
        vec4_t( 0.0f, f, 0.0f, 0.0f ),
        vec4_t( 0.0f, 0.0f, ( ( znear + zfar ) * range_rcp ), -1.0f ),
        vec4_t( 0.0f, 0.0f, ( ( ( znear * zfar ) * range_rcp ) * 2.0f ), 0.0f )
    );
}

mat44_t ProjectionDx11( const mat44_t & proj )
{
    const mat44_t sc = mat44_t::scale( vec3_t( 1, 1, 0.5f ) );
    const mat44_t tr = mat44_t::translation( vec3_t( 0, 0, 1 ) );
    return sc * tr * proj;
}

void ComputeMatrices( CameraMatrices* out, const CameraParams& params, const mat44_t& world )
{
    out->world = world;
    out->view = inverse( world );
    out->proj = PerspectiveMatrix( params.fov(), params.aspect(), params.znear, params.zfar );
    out->proj_api = ProjectionDx11( out->proj );
    
    //out->view_proj = out->proj * out->view;
}

void CameraInputContext::ReadInput( int mouseL, int mouseM, int mouseR, int mouseDx, int mouseDy, float mouseSensitivityInPix )
{
    const float mouse_dx_float = (float)mouseDx;
    const float mouse_dy_float = (float)mouseDy;

    const float new_left_y = (mouse_dy_float + mouse_dx_float) * mouseR;
    const float new_left_x = mouse_dx_float * mouseM;
    const float new_up_down = mouse_dy_float * mouseM;

    const float new_righ_x = mouse_dx_float * mouseL * mouseSensitivityInPix;
    const float new_righ_y = mouse_dy_float * mouseL * mouseSensitivityInPix;

    _left_x = new_left_x;
    _left_y = new_left_y;
    _right_x = new_righ_x;
    _right_y = new_righ_y;
    _up_down = new_up_down;
}

void CameraInputContext::UpdateInput( int mouseL, int mouseM, int mouseR, int mouseDx, int mouseDy, float mouseSensitivityInPix, float dt )
{
	const float mouse_dx_float = (float)mouseDx;
	const float mouse_dy_float = (float)mouseDy;

	const float new_left_y  = (mouse_dy_float + mouse_dx_float) * mouseR;
	const float new_left_x  = mouse_dx_float * mouseM;
	const float new_up_down = mouse_dy_float * mouseM;

	const float new_righ_x = mouse_dx_float * mouseL * mouseSensitivityInPix;
	const float new_righ_y = mouse_dy_float * mouseL * mouseSensitivityInPix;

	const float rc = 0.05f;
	_left_x  = LowPassFilter( new_left_x, _left_x, rc, dt );
	_left_y  = LowPassFilter( new_left_y, _left_y, rc, dt );
	_right_x = LowPassFilter( new_righ_x, _right_x, rc, dt );
	_right_y = LowPassFilter( new_righ_y, _right_y, rc, dt );
	_up_down = LowPassFilter( new_up_down, _up_down, rc, dt );
}

void CameraInputContext::UpdateInput( float analogX, float analogY, float dt )
{
	const float new_left_y = 0.f;
	const float new_left_x = analogX;
	const float new_upDown = analogY;

	const float new_right_x = 0.f;
	const float new_right_y = 0.f;

	const float rc = 0.1f;
	_left_x  = LowPassFilter( new_left_x, _left_x, rc, dt );
	_left_y  = LowPassFilter( new_left_y, _left_y, rc, dt );
	_right_x = LowPassFilter( new_right_x, _right_x, rc, dt );
	_right_y = LowPassFilter( new_right_y, _right_y, rc, dt );
	_up_down = LowPassFilter( new_upDown, _up_down, rc, dt );
}

bool CameraInputContext::AnyMovement() const
{
	const float sum =
		::fabsf( _left_x ) +
		::fabsf( _left_y ) +
		::fabsf( _right_x ) +
		::fabsf( _right_y ) +
		::fabsf( _up_down );

	return sum > 0.01f;
}

mat44_t CameraMovementFPP( const CameraInputContext & input, const mat44_t & world, float sensitivity )
{
	vec3_t dpos_ls( 0.f );
	dpos_ls += vec3_t::az() * input._left_y * sensitivity;
	dpos_ls += vec3_t::ax() * input._left_x * sensitivity;
	dpos_ls -= vec3_t::ay() * input._up_down * sensitivity;
	//bxLogInfo( "%f; %f", rightInputX, rightInputY );

	const float rot_dx( input._right_x );
	const float rot_dy( input._right_y );

	const mat33_t rotation( world.upper3x3() );

	const quat_t drot_y_ws = quat_t::rotationy( -rot_dx );
	const quat_t drot_x_ws = quat_t::rotation( -rot_dy, rotation.c0 );
	const quat_t drot_ws = drot_y_ws * drot_x_ws;

	const quat_t curr_camera_quat( rotation );

	const quat_t rot_ws = normalize( drot_ws * curr_camera_quat );
	const vec3_t pos_ws = mul_as_point( world, dpos_ls );

	return mat44_t( rot_ws, pos_ws );

}

void CameraInputFilter( CameraInputContext* output, const CameraInputContext& raw, const CameraInputContext& prev, f32 rc, f32 dt )
{
    output->_left_x  = LowPassFilter( raw._left_x , prev._left_x , rc, dt );
    output->_left_y  = LowPassFilter( raw._left_y , prev._left_y , rc, dt );
    output->_right_x = LowPassFilter( raw._right_x, prev._right_x, rc, dt );
    output->_right_y = LowPassFilter( raw._right_y, prev._right_y, rc, dt );
    output->_up_down = LowPassFilter( raw._up_down, prev._up_down, rc, dt );
}

void CameraInputLerp( CameraInputContext* output, f32 t, const CameraInputContext& a, const CameraInputContext& b )
{
    output->_left_x  = lerp( t, a._left_x, a._left_x );
    output->_left_y  = lerp( t, a._left_y, a._left_y );
    output->_right_x = lerp( t, a._right_x, a._right_x );
    output->_right_y = lerp( t, a._right_y, a._right_y );
    output->_up_down = lerp( t, a._up_down, a._up_down );
}
