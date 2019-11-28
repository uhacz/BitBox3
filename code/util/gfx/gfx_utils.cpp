#include "gfx_utils.h"
#include "../camera.h"
#include "../rdi_backend/rdi_backend.h"
#include "../rdix/rdix.h"
#include "../shaders/hlsl/samplers.h"

struct GFXUtilsData
{
    BXIAllocator* allocator = nullptr;
    struct
    {
        RDIXPipeline* copy_rgba = nullptr;
    }pipeline;

    ShaderSamplers samplers;
};
// ---
void GFXUtils::StartUp( GFXUtils** result, RDIDevice* dev, BXIAllocator* allocator )
{
    GFXUtils* u = (GFXUtils*)BX_MALLOC( allocator, sizeof( GFXUtils ) + sizeof( GFXUtilsData ), sizeof(void*) );
    u->data = new(u + 1)GFXUtilsData();

    GFXUtilsData* data = u->data;
    data->allocator = allocator;
    RDIXShaderFile* shader_file = LoadShaderFile( "shader/hlsl/bin/texture_util.shader", allocator );

    RDIXPipelineDesc pipeline_desc = {};
    pipeline_desc.Shader( shader_file, "copy_rgba" );
    data->pipeline.copy_rgba = CreatePipeline( dev, pipeline_desc, allocator );

    {// --- samplers
        RDISamplerDesc desc = {};

        desc.Filter( RDIESamplerFilter::NEAREST );
        data->samplers.point = CreateSampler( dev, desc );

        desc.Filter( RDIESamplerFilter::LINEAR );
        data->samplers.linear = CreateSampler( dev, desc );

        desc.Filter( RDIESamplerFilter::BILINEAR_ANISO );
        data->samplers.bilinear = CreateSampler( dev, desc );

        desc.Filter( RDIESamplerFilter::TRILINEAR_ANISO );
        data->samplers.trilinear = CreateSampler( dev, desc );
    }
    

    UnloadShaderFile( &shader_file, allocator );

    result[0] = u;
}

void GFXUtils::ShutDown( GFXUtils** result, RDIDevice* dev )
{
    if( !result[0] )
        return;

    GFXUtilsData* data = result[0]->data;

    {// --- samplers
        ::Destroy( &data->samplers.trilinear );
        ::Destroy( &data->samplers.bilinear );
        ::Destroy( &data->samplers.linear );
        ::Destroy( &data->samplers.point );
    }

    DestroyPipeline( &data->pipeline.copy_rgba );

    BXIAllocator* allocator = data->allocator;
    BX_DELETE0( allocator, result[0] );
}

void GFXUtils::CopyTexture( RDICommandQueue * cmdq, RDITextureRW * output, const RDITextureRW & input, float aspect )
{
    const RDITextureInfo& src_info = input.info;
    int32_t viewport[4] = {};
    RDITextureInfo dst_info = {};

    if( output )
    {
        dst_info = input.info;
    }
    else
    {
        RDITextureRW back_buffer = GetBackBufferTexture( cmdq );
        dst_info = back_buffer.info;
    }

    ComputeViewport( viewport, aspect, dst_info.width, dst_info.height, src_info.width, src_info.height );

    RDIXResourceBinding* resources = ResourceBinding( data->pipeline.copy_rgba );
    SetResourceRO( resources, "texture0", &input );
    if( output )
        ChangeRenderTargets( cmdq, output, 1, RDITextureDepth(), false );
    else
        ChangeToMainFramebuffer( cmdq );

    SetViewport( cmdq, RDIViewport::Create( viewport ) );
    BindPipeline( cmdq, data->pipeline.copy_rgba );
    Draw( cmdq, 6, 0 );
}

void GFXUtils::BindSamplers( RDICommandQueue* cmdq )
{
    SetSamplers( cmdq, (RDISampler*)&data->samplers.point, 0, 4, RDIEPipeline::ALL_STAGES_MASK );
}
