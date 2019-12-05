#ifndef SHADER_VIEW_DATA_H
#define SHADER_VIEW_DATA_H

struct CameraViewData
{
    float4x4 world;
    float4x4 view;
    float4x4 proj;
    float4x4 proj_api;
    float4x4 view_proj_api;

    float aspect;
    float fov;
    float znear;
    float zfar;
};

struct RenderTargetData
{
    float4 size;
    float4 size_rcp;
};

struct DrawCallData
{
    uint target_index;
    uint camera_index;
    uint vstream_begin;
    uint vstream_count;
    uint first_instance_index;
    uint3 __padding;
    DrawRange draw_range;
};

#endif