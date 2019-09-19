#ifndef MATERIAL_DATA_H
#define MATERIAL_DATA_H

struct Material
{
	float3 diffuse_albedo;
	float roughness;
	float3 specular_albedo;
	float metal;
};

#ifdef SHADER_IMPLEMENTATION
shared cbuffer _MaterialData
{
	Material _material[SHADER_MAX_MATERIALS];
};

Texture2D tex_material_base_color;
Texture2D tex_material_normal;
Texture2D tex_material_roughness;
Texture2D tex_material_metalness;

#endif

#endif