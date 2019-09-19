<pass name="base" vertex="vs_base" pixel="ps_base">
<hwstate 
    depth_test="1"
    depth_write="1"
/>
</pass>
<pass name = "base_with_skybox" vertex = "vs_base" pixel = "ps_base">
<hwstate
    depth_test = "1"
    depth_write = "1"
    />
<define USE_SKYBOX="1"/>
</pass>
<pass name="full" vertex="vs_base" pixel="ps_full">
<hwstate 
    depth_test="1"
    depth_write="1"
/>
<define USE_SKYBOX="1"/>
</pass>
#~header

struct IN_VS
{
	uint   instanceID : SV_InstanceID;
	float3 pos	  	  : POSITION;
	float3 nrm		  : NORMAL;
	float2 uv0		  : TEXCOORD0;
#if USE_TANGENTS == 1
    float3 tan        : TANGENT;
    float3 bin        : BINORMAL;
#endif
};

struct IN_PS
{
	float4 pos_hs	: SV_Position;
	float3 pos_ws	: POSITION;
	float3 nrm_ws	: NORMAL;
	float2 uv0		: TEXCOORD0;
#if USE_TANGENTS == 1
    float3 tan_ws   : TANGENT;
    float3 bin_ws   : BINORMAL;
#endif
};

struct OUT_PS
{
	float4 rgba : SV_Target0;
};

#define SHADER_IMPLEMENTATION

#include "common.h"

// --- tansform buffer
#include "transform_instance_data.h"
// --- frame
#include "frame_data.h"
// --- material
#include "material_data.h"
// --- samplers
#include "samplers.h"

IN_PS vs_base( IN_VS input )
{
	IN_PS output;

	// transform matrices rows
	float4 world_0, world_1, world_2;
	float3 world_it_0, world_it_1, world_it_2;
	
	LoadWorld( world_0, world_1, world_2, input.instanceID );
	LoadWorldIT( world_it_0, world_it_1, world_it_2, input.instanceID );

	float4 pos_ls = float4(input.pos, 1.0);
	float3 nrm_ls = input.nrm;
    float3 pos_ws = TransformPosition( world_0, world_1, world_2, pos_ls );

    uint camera_index = _idata.camera_index;

    output.pos_hs = mul( _camera.view_proj[camera_index], float4(pos_ws, 1.0) );
    output.pos_ws = pos_ws;
    output.nrm_ws = TransformNormal( world_it_0, world_it_1, world_it_2, nrm_ls );

#if USE_TANGENTS == 1
    output.tan_ws = TransformNormal( world_it_0, world_it_1, world_it_2, input.tan );
    output.bin_ws = TransformNormal( world_it_0, world_it_1, world_it_2, input.bin );
#endif

	output.uv0 = input.uv0;
	return output;
}

#define PI 3.1415926
#define PI_RCP 0.31830988618379067154

float WrappedNdotL( in float3 N, in float3 L, in float w )
{
	return saturate( (dot( N, L ) + w) / ((1 + w) * (1 + w)) );
}

struct LitInput
{
    float3 N;
    float3 L;
    float3 V;
    float3 H;
    float3 R;
    float NoL;
    float NoV;
    float NoH;
    float HoL;
};

struct LitData
{
    float3 diffuse;
    float3 specular;
};

LitInput ComputeLitInput( in float3 N, in float3 L, in float3 V )
{
    const float3 H = normalize( L + V );

    LitInput o;
    o.N = N;
    o.L = L;
    o.V = V;
    o.H = H;
    o.R = 2.0*dot( N, V )*N - V;
    o.NoL = saturate( dot( N, L ) );
    o.NoV = saturate( dot( N, V ) );
    o.NoH = saturate( dot( N, H ) );
    o.HoL = saturate( dot( H, L ) );
    return o;
}

// ================================================================================================
// Calculates the Fresnel factor using Schlick's approximation
// ================================================================================================
//float3 Fresnel( in float3 f0, in float HoL )
//{
//	//float lDotH = saturate( dot( l, h ) );
//	float3 fresnel = f0 + (1.0f - f0) * pow( (1.0f - HoL), 5.0f );
//    fresnel *= dot( f0, 1.0f ) > 0.0f;
//
//	return fresnel;
//}

float pow5( in float x )
{
    return x * pow( x, 4 );
}

float3 Fresnel( in float3 F0, in float cos_i, float smoothness )
{
    return lerp( F0, 1.0, 0.9 * pow5( (smoothness*smoothness) * (1.0 - max( 0.0, cos_i )) ) );
}

// ===============================================================================================
// Helper for computing the GGX visibility term
// ===============================================================================================
float GGX_V1( in float m2, in float nDotX )
{
	return 1.0f / (nDotX + sqrt( m2 + (1 - m2) * nDotX * nDotX ));
}

// ===============================================================================================
// Computes the specular term using a GGX microfacet distribution, with a matching
// geometry factor and visibility term. Based on "Microfacet Models for Refraction Through
// Rough Surfaces" [Walter 07]. m is roughness, n is the surface normal, h is the half vector,
// l is the direction to the light source, and specAlbedo is the RGB specular albedo
// ===============================================================================================
float GGX_Specular( in LitInput lin, in float roughness )
{
	float NoH2 = lin.NoH * lin.NoH;
	float m2 = roughness * roughness;

	// Calculate the distribution term
	float d = m2 / (PI * pow( NoH2 * (m2 - 1) + 1, 2.0f ));

	// Calculate the matching visibility term
	float v1i = GGX_V1( m2, lin.NoL );
	float v1o = GGX_V1( m2, lin.NoV );
	float vis = v1i * v1o;

	return d * vis;
}



LitData CalcDirectionalLighting( in LitInput lin, in Material mat )
{
    LitData lit_data = (LitData)0;

	float3 lighting = 0.0f;
	if( lin.NoL > 0.0f )
	{
		float specular = GGX_Specular( lin, mat.roughness );
		float3 fresnel = Fresnel( mat.specular_albedo, lin.HoL, 1.0 - mat.roughness );

        lit_data.diffuse = max( 0, mat.diffuse_albedo * PI_RCP * lin.NoL );
        lit_data.specular = max( 0, specular * fresnel * lin.NoL );
	}

    return lit_data;
}

#if USE_SKYBOX
LitData ComputeSkyBox( in LitData lit_data, in LitInput lin, in Material material )
{
    float a2 = material.roughness * material.roughness;
    float specPower = (1.0 / a2 - 1.0) * 2.0;

    float MIPlevel = log2( _ldata.environment_map_width * sqrt( 3 ) ) - 0.5 * log2( specPower + 1 );
    float3 diffuse_env = _skybox.SampleLevel( _samp_bilinear, lin.N, _ldata.environment_map_max_mip ).rgb;
    float3 specular_env = _skybox.SampleLevel( _samp_bilinear, lin.R, MIPlevel ).rgb;

    float3 F = Fresnel( material.specular_albedo, max( 0.001, lin.NoV ), 1.0 - material.roughness );
    float3 lambert_color = (1.0 - F) * (lit_data.diffuse) * PI_RCP;

    LitData lit_output;
    lit_output.diffuse = diffuse_env * lambert_color * (1.0 - F) * _ldata.sky_intensity;
    lit_output.specular = specular_env * F *_ldata.sky_intensity;

    return lit_output;
}
#endif

OUT_PS ps_base( IN_PS input )
{
    const float3 light_color = _ldata.sun_color;// float3(1, 1, 1);
    const float3 L = _ldata.sun_L;

    const uint camera_index = _idata.camera_index;
    const float3 V = normalize( _camera.eye[camera_index].xyz - input.pos_ws );
	const float3 N = normalize( input.nrm_ws );
	
    Material mat = _material[_idata.material_index];

    LitInput lit_input = ComputeLitInput( N, L, V );
    LitData lit_data = CalcDirectionalLighting( lit_input, mat );
   
    const float3 base_color = float3(1, 1, 1);

    LitData lit_sky = (LitData)0;
#if USE_SKYBOX
    lit_sky = ComputeSkyBox( lit_data, lit_input, mat );
#else
    float alpha = WrappedNdotL( N, L, 1.1f );
    float3 indirect = 0.1f * mat.diffuse_albedo * (alpha)* PI_RCP;
    lit_sky.diffuse = max( lit_data.diffuse, indirect );
#endif

    float3 color_diffuse = (lit_data.diffuse * base_color * light_color) + lit_sky.diffuse;
    float3 color_specular = (lit_data.specular * light_color) + lit_sky.specular;
    float3 color = color_diffuse + color_specular;

    OUT_PS output;
    output.rgba = float4(color, 1);
    return output;
}

float3x3 CotangentFrame( in float3 N, in float3 p, in float2 uv )
{
    // get edge vectors of the pixel triangle
    float3 dp1 = ddx( p );
    float3 dp2 = ddy( p );
    float2 duv1 = ddx( uv );
    float2 duv2 = ddy( uv );

    // solve the linear system
    float3 dp2perp = cross( dp2, N );
    float3 dp1perp = cross( N, dp1 );
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    // construct a scale-invariant frame 
    float invmax = rsqrt( max( dot( T, T ), dot( B, B ) ) );
    return ( float3x3(T * invmax, B * invmax, N) );
}

float3 PerturbNormal( in float3 N, in float3 p, in float2 texcoord )
{
    float3 map = ( tex_material_normal.Sample( _samp_point, texcoord ).xyz * 2.0 - 1.0 );
    float3x3 TBN = CotangentFrame( N, normalize(p), texcoord );
    return normalize( mul( map, TBN ) );
}

float3 PerturbNormal_ref( in float3 N, in float3 T, in float3 B, in float2 texcoord )
{
    float3 map = normalize( tex_material_normal.Sample( _samp_point, texcoord ).xyz * 2.0 - 1.0 );
    float3x3 TBN = transpose( float3x3( T, B, N ) );
    float3 NN = mul( TBN, map );
    return normalize( NN );
}

OUT_PS ps_full( IN_PS input )
{
    const uint camera_index = _idata.camera_index;
    //const float3 light_pos = float3(-5.f, 5.f, 0.f);
    const float3 light_color = _ldata.sun_color;// float3(1, 1, 1);
    const float3 L = _ldata.sun_L;
    const float3 V = normalize( _camera.eye[camera_index].xyz - input.pos_ws.xyz );
    const float3 N_input = normalize( input.nrm_ws );
    
#if USE_TANGENTS == 1
    const float3 T_input = normalize( input.tan_ws );
    const float3 B_input = normalize( input.bin_ws );
    const float3 N = PerturbNormal_ref( N_input, T_input, B_input, input.uv0 );
#else
    const float3 N = PerturbNormal( N_input, input.pos_ws, input.uv0 );
#endif

    LitInput lit_input = ComputeLitInput( N, L, V );
    
    Material mat = _material[_idata.material_index];
    mat.roughness *= tex_material_roughness.Sample( _samp_point, input.uv0 ).r;
    mat.metal *= tex_material_metalness.Sample( _samp_point, input.uv0 ).r;
    mat.specular_albedo = lerp( 0.04, mat.specular_albedo, mat.metal );
    
    const float3 base_color = tex_material_base_color.Sample( _samp_trilinear, input.uv0 ) * saturate( 1.f - mat.metal );

    LitData lit_data = CalcDirectionalLighting( lit_input, mat );
    LitData lit_sky = (LitData)0;
#if USE_SKYBOX
    lit_sky = ComputeSkyBox( lit_data, lit_input, mat );
#endif

    float3 color_diffuse = ( lit_data.diffuse * base_color * light_color ) + lit_sky.diffuse;
    float3 color_specular = (lit_data.specular * light_color) + lit_sky.specular;
    float3 color = color_diffuse + color_specular;
    
    OUT_PS output;
    output.rgba = float4(color, 1 );
    return output;
}