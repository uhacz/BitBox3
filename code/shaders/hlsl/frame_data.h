#ifndef MATERIAL_FRAME_DATA
#define MATERIAL_FRAME_DATA

#define GLOBAL_FRAME_DATA_SLOT 1
#define CAMERA_DATA_SLOT 2
#define MATERIAL_DATA_SLOT 3
#define LIGHTING_FRAME_DATA_SLOT 4
#define LIGHTING_SKY_CUBEMAP_SLOT 6

#define SHADER_MAX_CAMERAS 32
#define SHADER_MAX_MATERIALS 32


struct CameraData
{
    float4x4 world[SHADER_MAX_CAMERAS];
    float4x4 view[SHADER_MAX_CAMERAS];
    float4x4 view_proj[SHADER_MAX_CAMERAS];
    float4x4 view_proj_inv[SHADER_MAX_CAMERAS];
    float4   eye[SHADER_MAX_CAMERAS];
    float4   dir[SHADER_MAX_CAMERAS];
};

struct GlobalFrameData
{
    float2 rtarget_size;
    float2 rtarget_size_rcp;
    uint main_camera_index;
};

struct LightningFrameData
{
    float4 sun_L; // L means direction TO light
    float4 sun_color; 
    float sun_intensity;
    float sky_intensity;
    float environment_map_width;
    int environment_map_max_mip;
};


#ifdef SHADER_IMPLEMENTATION

shared cbuffer _Camera : register(BSLOT( CAMERA_DATA_SLOT ))
{
    CameraData _camera;
};

shared cbuffer _GlobalFrameData : register(BSLOT( GLOBAL_FRAME_DATA_SLOT ))
{
    GlobalFrameData _fdata;
};

shared cbuffer _Lighting : register(BSLOT( LIGHTING_FRAME_DATA_SLOT ))
{
    LightningFrameData _ldata;
};

#if USE_SKYBOX
textureCUBE<float3> _skybox : register(TSLOT( LIGHTING_SKY_CUBEMAP_SLOT ));
#endif


#endif


#endif