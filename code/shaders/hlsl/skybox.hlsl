<pass name = "skybox" vertex = "vs_fullscreenquad" pixel = "ps_skybox">
<hwstate
    depth_test = "0"
    depth_write = "0" />
    <define USE_SKYBOX = "1" />
    </pass>
#~header

#define SHADER_IMPLEMENTATION

#include "common.h"
#include "fullscreenquad.hlsl"
#include "frame_data.h"
#include "samplers.h"

#define IN_PS out_VS_screenquad

float4 ps_skybox( IN_PS input ) : SV_Target0
{
    float2 texel_ext = _fdata.rtarget_size_rcp * 0.5;
    float4 position_cs = float4( (input.uv + texel_ext) * float2(2.0, -2.0) + float2(-1.0, 1.0), 1.0, 1.0);
    float4 position_ws = mul( _camera.view_proj_inv[_fdata.main_camera_index], position_cs );
    position_ws.xyz *= rcp( position_ws.w );

    float3 V = normalize( _camera.eye[_fdata.main_camera_index].xyz - position_ws.xyz );
    V.xy = -V.xy;

    float3 skybox_color = _skybox.SampleLevel( _samp_linear, V, 0 );
    
    return float4(skybox_color, 1.0 );
}
