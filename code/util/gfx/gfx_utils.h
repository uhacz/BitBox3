#pragma once

struct BXIAllocator;
struct RDIDevice;
struct RDICommandQueue;
struct RDITextureRW;

struct GFXUtilsData;
struct GFXUtils
{
    static void StartUp( GFXUtils** result, RDIDevice* dev, BXIAllocator* allocator );
    static void ShutDown( GFXUtils** result, RDIDevice* dev );

    void CopyTexture( RDICommandQueue* cmdq, RDITextureRW* output, const RDITextureRW& input, float aspect );
    void BindSamplers( RDICommandQueue* cmdq );

    GFXUtilsData* data = nullptr;
};
