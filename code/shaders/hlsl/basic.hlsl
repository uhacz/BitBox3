<pass name = "main" vertex = "vs_main" pixel = "ps_main">
</pass>
#~header

#define SHADER_IMPLEMENTATION
#include "vertex_layout.h"
#include "view_data.h"
#include "common.h"

StructuredBuffer<RenderTargetData>  g_target    : TSLOT( 0 );
StructuredBuffer<CameraViewData>    g_camera    : TSLOT( 1 );
StructuredBuffer<float4x4>          g_matrix    : TSLOT( 2 );
StructuredBuffer<VertexStream>      g_vstream   : TSLOT( 3 );

shared cbuffer DrawCallDataCB : BSLOT( 16 )
{
    DrawCallData g_draw_call;
};

ByteAddressBuffer g_vertices    : TSLOT( 32 );
ByteAddressBuffer g_indices     : TSLOT( 40 );

struct in_VS
{
	uint   instanceID : SV_InstanceID;
	float4 pos	  	  : POSITION;
};

struct in_PS
{
    float4 h_pos	: SV_Position;
	float4 color	: TEXCOORD0;
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
    OUT.rgba = input.color;
    return OUT;
}

#define POS_ATTRIB 0
#define COLOR_ATTRIB 1

in_PS vs_main( uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID )
{
    in_PS output;
    
    const uint instance_index = g_draw_call.first_instance_index + instance_id;
    matrix wm = g_matrix[instance_index];

    uint colorU32 = asuint( wm[3][3] );
    wm[3][3] = 1.0f;

    VertexStream pos_stream = g_vstream[g_draw_call.vstream_begin];

    uint vertex_index = LoadVertexIndex( g_draw_call.draw_range, vertex_id, g_indices );
    float3 pos_ls = VertexLoad3F( pos_stream, POS_ATTRIB, vertex_index, g_vertices );

    CameraViewData camera_data = g_camera[g_draw_call.camera_index];

    float4 world_pos = mul( wm, float4(pos_ls, 1.0) );
    output.h_pos = mul( camera_data.view_proj_api, world_pos );
    output.color = colorU32toFloat4_RGBA( colorU32 );
    return output;
}
