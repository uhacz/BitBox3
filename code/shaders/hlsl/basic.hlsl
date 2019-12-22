<pass name = "main" vertex = "vs_main" pixel = "ps_main">
</pass>
#~header

#define SHADER_IMPLEMENTATION
#include "vertex_layout.h"
#include "view_data.h"
#include "common.h"

shared StructuredBuffer<RenderTargetData>  _target    : register( t0 );
shared StructuredBuffer<CameraViewData>    _camera    : register( t1 );
shared StructuredBuffer<float4x4>          _matrix    : register( t2 );
shared StructuredBuffer<VertexStream>      _vstream   : register( t3 );

shared cbuffer _DrawCallDataCB : register( b0 )
{
    DrawCallData _draw_call;
};

ByteAddressBuffer _vertices    : register( t32 );
ByteAddressBuffer _indices     : register( t40 );

struct in_VS
{
	uint   instanceID : SV_InstanceID;
	float4 pos	  	  : POSITION;
};

struct in_PS
{
    float4 h_pos	: SV_Position;
    float3 w_nrm    : NORMAL;
	//float4 color	: TEXCOORD0;
};

struct out_PS
{
	float4 rgba : SV_Target0;
};

float4 colorU32toFloat4_RGBA( uint rgba )
{
    float r = (float)((rgba >> 24) & 0xFF);
    float g = (float)((rgba >> 16) & 0xFF);
    float b = (float)((rgba >> 8) & 0xFF);
    float a = (float)((rgba >> 0) & 0xFF);

    const float scaler = 0.00392156862745098039; // 1/255
    return float4(r, g, b, a) * scaler;
}

out_PS ps_main( in_PS input )
{
	out_PS OUT;
    //OUT.rgba = input.color;

    float3 N = normalize( input.w_nrm );
    float d = saturate( dot( N, normalize( float3(1, 1, 1) ) ) );

    OUT.rgba = float4(d.xxx, 1);
    return OUT;
}

#define POSITION_ATTRIB 0
#define NORMAL_ATTRIB 1

in_PS vs_main( uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID )
{
    in_PS output;
    
    const uint instance_index = _draw_call.first_instance_index + instance_id;
    matrix wm = _matrix[instance_index];

    VertexStream pos_stream = _vstream[_draw_call.vstream_begin];

    uint vertex_index = LoadVertexIndex( _draw_call.draw_range, vertex_id, _indices );
    float3 pos_ls = VertexLoad3F( pos_stream, POSITION_ATTRIB, vertex_index, _vertices );
    float3 nrm_ls = VertexLoad3F( pos_stream, NORMAL_ATTRIB, vertex_index, _vertices );
    
    CameraViewData camera_data = _camera[_draw_call.camera_index];

    float4 world_pos = mul( wm, float4(pos_ls, 1.0) );
    float3 world_nrm = mul( (float3x3)wm, nrm_ls );

    output.h_pos = mul( camera_data.view_proj_api, world_pos );
    output.w_nrm = world_nrm;
    //output.color = colorU32toFloat4_RGBA( colorU32 );
    return output;
}
