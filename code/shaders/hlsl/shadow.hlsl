<pass name = "shadow_ss" vertex = "vs_fullscreenquad" pixel = "ps_shadow_ss">
<hwstate
    depth_test = "0"
    depth_write = "0" />
</pass>

<pass name = "shadow_combine" vertex = "vs_fullscreenquad" pixel = "ps_shadow_combine">
<hwstate
    depth_test = "0"
    depth_write = "0" />
</pass>

<pass name="shadow_depth" vertex = "vs_shadow_depth" pixel = "ps_shadow_depth" >
<hwstate depth_test = "1" depth_write = "1" color_mask = "" />
</pass>

#~header

#define SHADER_IMPLEMENTATION

#include "common.h"
#include "fullscreenquad.hlsl"
#include "shadow_data.h"
#include "samplers.h"

#define IN_PS out_VS_screenquad

Texture2D<float> depth_tex  : register(t0);

shared cbuffer _Frame : register(BSLOT( SHADOW_DATA_SLOT ))
{
    ShadowData _fdata;
};

int3 ComputePositionSS( float2 uv, float2 rt_size )
{
    return int3((int2)(uv * rt_size), 0);
}
float4 ComputePositionCS( in int3 position_ss, in float depth_cs, in float2 rt_size_rcp )
{
    return float4(((float2(position_ss.xy) + 0.5) * rt_size_rcp) * float2(2.0, -2.0) + float2(-1.0, 1.0), depth_cs, 1.0);
}

float TraceWS( in float3 begin_ws, in float3 end_ws, in float step, in float depth_cs )
{
    float t = step;
    while( t <= 1.0 )
    {
        float3 sample_pos_ws = lerp( begin_ws, end_ws, t );
        float4 sample_pos_hs = mul( _fdata.camera_view_proj, float4(sample_pos_ws, 1.0) );
        sample_pos_hs *= rcp( sample_pos_hs.w );

        float2 sample_uv = sample_pos_hs.xy * 0.5f + 0.5f;
        sample_uv.y = 1.f - sample_uv.y;
        int2 sample_ss = (int2)(sample_uv * _fdata.rtarget_size);
        float sample_depth_cs = depth_tex.Load( int3(sample_ss, 0) );

        if( sample_depth_cs > depth_cs )
            break;

        t += step;
    }

    return t;
}

float ps_shadow_ss( IN_PS input ) : SV_Target0
{
    int3   position_ss = ComputePositionSS( input.uv, _fdata.rtarget_size );
    float  depth_cs = depth_tex.Load( position_ss );
    float4 position_cs = ComputePositionCS( position_ss, depth_cs, _fdata.rtarget_size_rcp );
    //float4 position_cs = float4(((float2(position_ss.xy) + 0.5) * _fdata.rtarget_size_rcp) * float2(2.0, -2.0) + float2(-1.0, 1.0), depth_cs, 1.0);

    if( depth_cs > 0.99999f )
        return (float4)1;

    float4 position_ws = mul( _fdata.camera_view_proj_inv, position_cs );
    position_ws.xyz *= rcp( position_ws.w );

    const float num_steps = 64;
    const float tstep = 1.f / num_steps;
    
    float shadow_value = 0.f;
    float t = tstep;
    while( t <= 1.f )
    {
        float3 sample_pos_ws = lerp( position_ws, _fdata.light_pos_ws.xyz, t*t*t*t );
        float4 sample_pos_hs = mul( _fdata.camera_view_proj, float4(sample_pos_ws, 1.0) );
        sample_pos_hs *= rcp( sample_pos_hs.w );

        float2 sample_uv =  sample_pos_hs.xy * 0.5f + 0.5f;
        sample_uv.y = 1.f - sample_uv.y;
        int2 sample_ss = (int2)(sample_uv * _fdata.rtarget_size );
        float sample_depth_cs = depth_tex.Load( int3( sample_ss, 0 ) );

        shadow_value += step( 0.0001f, saturate( sample_pos_hs.z - sample_depth_cs ) );
        t += tstep;
    }
    shadow_value *= rcp( num_steps );
    shadow_value = clamp( 1 - shadow_value, 0.25, 1.0 );
    return shadow_value;
}

Texture2D<float> shadow_tex  : register(t0);
Texture2D<float4> color_tex  : register(t1);

float4 ps_shadow_combine( IN_PS input ) : SV_Target0
{
    int3   position_ss = ComputePositionSS( input.uv, _fdata.rtarget_size );

    float shadow = shadow_tex.Load( position_ss );
    float4 color = color_tex.Load( position_ss );

    return color * shadow;
}


// --- tansform buffer
#include "transform_instance_data.h"

struct IN_VS_SHADOW_DEPTH
{
    uint   instanceID : SV_InstanceID;
    float3 pos	  	  : POSITION;
};
struct IN_PS_SHADOW_DEPTH
{
    float4 pos_hs	: SV_Position;
};
struct OUT_PS_SHADOW_DEPTH
{};

IN_PS_SHADOW_DEPTH vs_shadow_depth( in IN_VS_SHADOW_DEPTH input )
{
    IN_PS_SHADOW_DEPTH output = (IN_PS_SHADOW_DEPTH)0;

    float4 world_0, world_1, world_2;
    LoadWorld( world_0, world_1, world_2, input.instanceID );
    float4 pos_ls = float4(input.pos, 1.0);
    float3 pos_ws = TransformPosition( world_0, world_1, world_2, pos_ls );

    output.pos_hs = mul( _fdata.camera_view_proj, float4(pos_ws, 1.0) );
    return output;
}

[earlydepthstencil]
OUT_PS_SHADOW_DEPTH ps_shadow_depth( IN_PS_SHADOW_DEPTH input ) 
{
    OUT_PS_SHADOW_DEPTH output;
    return output;
}