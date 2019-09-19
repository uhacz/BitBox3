#ifndef SHADER_FULLSCREENQUAD
#define SHADER_FULLSCREENQUAD

struct out_VS_screenquad
{
	noperspective float2 screenPos : TEXCOORD0;
	noperspective float2 uv : TEXCOORD1;
};

static const float x_coord[6] = { -1, 1,-1,  -1, 1, 1 };
static const float y_coord[6] = { -1,-1, 1,   1,-1, 1 };

out_VS_screenquad vs_fullscreenquad(
	in uint vertexID : SV_VertexID,
	out float4 OUT_hpos : SV_Position )
{
	float2 xy = float2( x_coord[vertexID], y_coord[vertexID] );
	
	OUT_hpos = float4( xy, 0.0, 1.0 );

	out_VS_screenquad output;
	output.screenPos = xy;
	output.uv = xy * 0.5 + 0.5;
	output.uv.y = 1.0 - output.uv.y;
	return output;
}

#endif