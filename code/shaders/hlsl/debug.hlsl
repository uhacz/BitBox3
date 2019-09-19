<pass name="object_D_W" vertex="vs_object" pixel="ps_main">
<hwstate 
    depth_test="1"
    depth_write="1"
    fill_mode="WIREFRAME"
/>
</pass>
<pass name="object_ND_W" vertex="vs_object" pixel="ps_main">
<hwstate 
    depth_test="0"
    depth_write="0"
    fill_mode="WIREFRAME"
/>
</pass>
<pass name="object_D_S" vertex="vs_object" pixel="ps_main">
<hwstate 
    depth_test="1"
    depth_write="1"
    fill_mode="SOLID"
/>
</pass>
<pass name="object_ND_S" vertex="vs_object" pixel="ps_main">
<hwstate 
    depth_test="0"
    depth_write="0"
    fill_mode="SOLID"
/>
</pass>
<pass name="lines_D" vertex="vs_lines" pixel="ps_main">
<hwstate 
    depth_test="1"
    depth_write="1"
    fill_mode="WIREFRAME"
/>
</pass>
<pass name="lines_ND" vertex="vs_lines" pixel="ps_main">
<hwstate 
    depth_test="0"
    depth_write="0"
    fill_mode="WIREFRAME"
/>
</pass>

#~header

StructuredBuffer<matrix> g_matrices;

shared cbuffer MaterialData
{
    matrix view_proj_matrix;
    
};
shared cbuffer InstanceData
{
    uint instance_batch_offset;
};

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

in_PS vs_object( in_VS input )
{
    in_PS output;
    const uint instance_index = instance_batch_offset + input.instanceID;
	matrix wm = g_matrices[instance_index];

    uint colorU32 = asuint( wm[3][3] );
    wm[3][3] = 1.0f;
    
	float4 world_pos = mul( wm, float4( input.pos.xyz, 1.0 ) );
    output.h_pos = mul( view_proj_matrix, world_pos );
    output.color = colorU32toFloat4_RGBA( colorU32 );
    return output;
}

in_PS vs_lines( in_VS input )
{
	in_PS output;
	uint color_u32 = asuint( input.pos.w );
    float4 world_pos = float4( input.pos.xyz, 1.0 );
	output.h_pos = mul( view_proj_matrix, world_pos );
    output.color = colorU32toFloat4_RGBA( color_u32 );
	return output;
}

out_PS ps_main( in_PS input )
{
	out_PS OUT;
	OUT.rgba = input.color;
    return OUT;
}
