#ifndef SHADER_SHADOW_DATA_H
#define SHADER_SHADOW_DATA_H

#define SHADOW_DATA_SLOT 10

struct ShadowData
{
    float4x4 camera_world;
    float4x4 camera_view_proj;
    float4x4 camera_view_proj_inv;
    float4 light_pos_ws;
    float2 rtarget_size;
    float2 rtarget_size_rcp;
};

#endif