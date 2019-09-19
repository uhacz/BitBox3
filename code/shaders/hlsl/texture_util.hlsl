<pass name="copy_rgba" vertex="vs_fullscreenquad" pixel="ps_copy_rgba">
<hwstate
	depth_test = "0"
	depth_write = "0"/>
</pass>
#~header

#define SHADER_IMPLEMENTATION

#include "fullscreenquad.hlsl"
#include "samplers.h"

#define IN_PS out_VS_screenquad

Texture2D texture0 : register(t0);
Texture2D texture1 : register(t1);

float4 ps_copy_rgba( IN_PS input ) : SV_Target0
{

	return texture0.SampleLevel( _samp_point, input.uv, 0 ).xyzw;
}
