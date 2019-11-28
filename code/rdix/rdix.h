#pragma once

#include <foundation/type.h>
//#include <initializer_list>

#include "rdix_type.h"
#include <util/par_shapes/par_shapes.h>
#include <util/poly_shape/poly_shape.h>

struct RDICommandQueue;
struct RDIDevice;
struct RDIXCommand;
struct RDIXCommandBuffer;
struct BXIAllocator;

// --- Shader
RDIXShaderFile* LoadShaderFile  ( const char* name, BXIAllocator* allocator );
RDIXShaderFile* CreateShaderFile( const void* data, uint32_t dataSize );
void		    UnloadShaderFile( RDIXShaderFile** shaderfile, BXIAllocator* allocator );
uint32_t		GenerateShaderFileHashedName( const char* name, uint32_t version );
uint32_t		FindShaderPass( RDIXShaderFile* shaderfile, const char* passname );

// --- Pipeline
struct RDIXPipeline
{
    RDIShaderPass pass;
    RDIHardwareState hardware_state;
    RDIInputLayout input_layout;
    RDIXResourceBinding* resources = nullptr;
    RDIETopology::Enum topology = RDIETopology::TRIANGLES;
    BXIAllocator* allocator = nullptr;
};

RDIXPipeline*		 CreatePipeline ( RDIDevice* dev, const RDIXPipelineDesc& desc, BXIAllocator* allocator );
void				 DestroyPipeline( RDIXPipeline** pipeline );

union RDIXPipelineFlags
{
    u32 all = 0xFFFFFFFF;
    struct  
    {
        u32 shaders : 1;
        u32 resources : 1;
        u32 hw_state : 1;
    };


    RDIXPipelineFlags& NoShaders() { shaders = 0; return *this; }
    RDIXPipelineFlags& NoResources() { resources = 0; return *this; }
    RDIXPipelineFlags& NoHwState() { hw_state = 0; return *this; }
};
void				 BindPipeline( RDICommandQueue* cmdq, RDIXPipeline* pipeline, const RDIXPipelineFlags& flags = RDIXPipelineFlags() );
RDIXResourceBinding* ResourceBinding( const RDIXPipeline* p );


// --- Resources
RDIXResourceBinding* CreateResourceBinding ( const RDIXResourceLayout& layout, BXIAllocator* allocator );
void				 DestroyResourceBinding( RDIXResourceBinding** binding );
u32                  GetMemorySize( const RDIXResourceBinding* binding );
void                 CloneResourceBinding( void* dst, u32 dst_size, const RDIXResourceBinding* binding );
RDIXResourceBinding* CloneResourceBinding( const RDIXResourceBinding* binding, BXIAllocator* allocator );
bool				 ClearResource( RDICommandQueue* cmdq, RDIXResourceBinding* binding, const char* name );
void				 BindResources( RDICommandQueue* cmdq, RDIXResourceBinding* binding );
uint32_t			 FindResource( RDIXResourceBinding* binding, const char* name );
void				 SetResourceROByIndex( RDIXResourceBinding* binding, uint32_t index, const RDIResourceRO* resource );
void				 SetResourceRWByIndex( RDIXResourceBinding* binding, uint32_t index, const RDIResourceRW* resource );
void				 SetConstantBufferByIndex( RDIXResourceBinding* binding, uint32_t index, const RDIConstantBuffer* cbuffer );
void				 SetSamplerByIndex( RDIXResourceBinding* binding, uint32_t index, const RDISampler* sampler );
bool				 SetResourceRO( RDIXResourceBinding* binding, const char* name, const RDIResourceRO* resource );
bool				 SetResourceRW( RDIXResourceBinding* binding, const char* name, const RDIResourceRW* resource );
bool				 SetConstantBuffer( RDIXResourceBinding* binding, const char* name, const RDIConstantBuffer* cbuffer );
bool				 SetSampler( RDIXResourceBinding* binding, const char* name, const RDISampler* sampler );

uint32_t			 GenerateResourceHashedName( const char* name );
RDIXResourceBindingMemoryRequirments CalculateResourceBindingMemoryRequirments( const RDIXResourceLayout& layout );



// --- RenderTarget
RDIXRenderTarget* CreateRenderTarget ( RDIDevice* dev, const RDIXRenderTargetDesc& desc, BXIAllocator* allocator );
void			  DestroyRenderTarget( RDIDevice* dev, RDIXRenderTarget** renderTarget );
void			  ClearRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* rtarget, float rgbad[5] );
void			  ClearRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* rtarget, float r, float g, float b, float a, float d );
void			  ClearRenderTargetDepth( RDICommandQueue* cmdq, RDIXRenderTarget* rtarget, float d );
void			  BindRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* renderTarget, const std::initializer_list<uint8_t>& colorTextureIndices, bool useDepth );
void			  BindRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* renderTarget, uint8_t color_texture_mask, bool use_depth );
void			  BindRenderTarget( RDICommandQueue* cmdq, RDIXRenderTarget* renderTarget );
RDITextureRW	  Texture( RDIXRenderTarget* rtarget, uint32_t index );
RDITextureDepth   TextureDepth( RDIXRenderTarget* rtarget );

// --- RenderSource
RDIXRenderSource* CreateRenderSource ( RDIDevice* dev, const RDIXRenderSourceDesc& desc, BXIAllocator* allocator );
RDIXRenderSource* CloneForCPUSkinning( RDIDevice* dev, const RDIXRenderSource* base, uint32_t skinned_slot_mask = RDIEVertexSlot::SkinningMaskPosNrm() );
RDIXRenderSource* CloneForGPUSkinning( RDIDevice* dev, const RDIXRenderSource* base, uint32_t skinned_slot_mask = RDIEVertexSlot::SkinningMaskPosNrm() );
RDIXRenderSource* CreateRenderSourceFromShape( RDIDevice* dev, const par_shapes_mesh* shape, BXIAllocator* allocator );
RDIXRenderSource* CreateRenderSourceFromShape( RDIDevice* dev, const poly_shape_t* shape, BXIAllocator* allocator );
RDIXRenderSource* CreateRenderSourceFromMemory( RDIDevice* dev, const RDIXMeshFile* header, BXIAllocator* allocator );
void			  DestroyRenderSource( RDIXRenderSource** rsource );
void			  BindRenderSource( RDICommandQueue* cmdq, RDIXRenderSource* renderSource );
void			  SubmitRenderSource( RDICommandQueue* cmdq, RDIXRenderSource* renderSource, uint32_t rangeIndex = 0 );
void			  SubmitRenderSource( RDICommandQueue* cmdq, RDIXRenderSource* renderSource, const RDIXRenderSourceRange& range );

void			  SubmitRenderSourceInstanced( RDICommandQueue* cmdq, RDIXRenderSource* renderSource, uint32_t numInstances, uint32_t rangeIndex = 0 );


uint32_t             NumVertexBuffers( const RDIXRenderSource* rsource );
uint32_t             NumVertices     ( const RDIXRenderSource* rsource );
uint32_t             NumIndices      ( const RDIXRenderSource* rsource );
uint32_t             NumRanges       ( const RDIXRenderSource* rsource );
RDIVertexBuffer      FindVertexBuffer( const RDIXRenderSource* rsource, RDIEVertexSlot::Enum slot );
RDIVertexBuffer      VertexBuffer    ( const RDIXRenderSource* rsource, uint32_t index );
RDIIndexBuffer       IndexBuffer     ( const RDIXRenderSource* rsource );
RDIXRenderSourceRange Range          ( const RDIXRenderSource* rsource, uint32_t index );

// --- TransformBuffer
struct RDIXTransformBufferBindInfo
{
    uint32_t instance_offset_slot = 0;
    uint32_t matrix_start_slot = 0;
    uint32_t stage_mask = RDIEPipeline::VERTEX_MASK;
};

RDIXTransformBuffer* CreateTransformBuffer( RDIDevice* dev, const RDIXTransformBufferDesc& desc, BXIAllocator* allocator );
void				 DestroyTransformBuffer( RDIXTransformBuffer** buffer, BXIAllocator* allocator );
void				 ClearTransformBuffer( RDIXTransformBuffer* buffer );
uint32_t             GetDataCapacity( RDIXTransformBuffer* buffer );
uint32_t			 AppendMatrix( RDIXTransformBuffer* buffer, const struct mat44_t& matrix );

void				 UploadTransformBuffer( RDICommandQueue* cmdq, RDIXTransformBuffer* buffer );
RDIXCommand*		 UploadTransformBuffer( RDIXCommandBuffer* cmdbuff, RDIXCommand* parentcmd, RDIXTransformBuffer* buffer );

void				 BindTransformBuffer( RDICommandQueue* cmdq, RDIXTransformBuffer* buffer, const RDIXTransformBufferBindInfo& bind_info );
RDIXCommand*		 BindTransformBuffer( RDIXCommandBuffer* cmdbuff, RDIXCommand* parentcmd, RDIXTransformBuffer* buffer, const RDIXTransformBufferBindInfo& bind_info );

struct RDIXTransformBufferCommands
{
	RDIXCommand* first;
	RDIXCommand* last;
};
RDIXTransformBufferCommands UploadAndSetTransformBuffer( RDIXCommandBuffer* cmdbuff, RDIXCommand* parentcmd, RDIXTransformBuffer* buffer, const RDIXTransformBufferBindInfo& bind_info );
RDIConstantBuffer GetInstanceOffsetCBuffer( RDIXTransformBuffer* cmdbuff );