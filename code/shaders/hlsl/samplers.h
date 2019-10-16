#ifndef SAMPLERS_HLSL
#define SAMPLERS_HLSL


#ifdef SHADER_IMPLEMENTATION
SamplerState _samp_point : register(s0);
SamplerState _samp_linear : register(s1);
SamplerState _samp_bilinear : register(s2);
SamplerState _samp_trilinear : register(s3);
#else

struct ShaderSamplers
{
	RDISampler point;
	RDISampler linear;
	RDISampler bilinear;
	RDISampler trilinear;
};

#endif
#endif