#pragma once

#include "rdi_backend_type.h"

struct RDIShaderReflection;

//
struct RDICommandQueue;
struct RDIDevice;
struct BXIAllocator;

void Startup( RDIDevice** dev, RDICommandQueue** cmdq, uintptr_t hWnd, int winWidth, int winHeight, int fullScreen, BXIAllocator* allocator );
void Shutdown( RDIDevice** dev, RDICommandQueue** cmdq, BXIAllocator* allocator );

RDIVertexBuffer	  CreateVertexBuffer  ( RDIDevice* dev, const RDIVertexBufferDesc& desc, uint32_t numElements, const void* data = 0 );
RDIIndexBuffer	  CreateIndexBuffer   ( RDIDevice* dev, RDIEType::Enum dataType, uint32_t numElements, const void* data = 0 );
RDIConstantBuffer CreateConstantBuffer( RDIDevice* dev, uint32_t sizeInBytes, const void* data = nullptr );
RDIBufferRO       CreateBufferRO      ( RDIDevice* dev, int numElements, RDIFormat format, unsigned cpuAccessFlag );
RDIBufferRO       CreateStructuredBufferRO( RDIDevice* dev, uint32_t numElements, uint32_t elementStride, unsigned cpuAccessFlag );


RDIShaderPass	  CreateShaderPass	( RDIDevice* dev, const RDIShaderPassCreateInfo& info );

RDITextureRO	CreateTextureFromDDS( RDIDevice* dev, const void* dataBlob, size_t dataBlobSize );
RDITextureRO	CreateTextureFromHDR( RDIDevice* dev, const void* dataBlob, size_t dataBlobSize );
RDITextureRW	CreateTexture1D     ( RDIDevice* dev, int w, int mips, RDIFormat format, unsigned bindFlags, unsigned cpuaFlags, const void* data );
RDITextureRW	CreateTexture2D     ( RDIDevice* dev, int w, int h, int mips, RDIFormat format, unsigned bindFlags, unsigned cpuaFlags, const void* data );
RDITextureDepth	CreateTexture2Ddepth( RDIDevice* dev, int w, int h, int mips, RDIEType::Enum dataType );
RDISampler		CreateSampler       ( RDIDevice* dev, const RDISamplerDesc& desc );

RDIInputLayout   CreateInputLayout  ( RDIDevice* dev, const RDIVertexLayout vertexLayout, RDIShaderPass shaderPass );
RDIHardwareState CreateHardwareState( RDIDevice* dev, RDIHardwareStateDesc desc );

void Destroy( RDIVertexBuffer* id );
void Destroy( RDIIndexBuffer* id );
void Destroy( RDIInputLayout * id );
void Destroy( RDIConstantBuffer* id );
void Destroy( RDIBufferRO* id );
void Destroy( RDIShaderPass* id );
void Destroy( RDITextureRO* id );
void Destroy( RDITextureRW* id );
void Destroy( RDITextureDepth* id );
void Destroy( RDISampler* id );
void Destroy( RDIBlendState* id );
void Destroy( RDIDepthState* id );
void Destroy( RDIRasterState * id );
void Destroy( RDIHardwareState* id );

// ---
void GetAPIDevice( RDIDevice* dev, ID3D11Device** apiDev, ID3D11DeviceContext** apiCtx );
RDICommandQueue* GetImmediateCommandQueue( RDIDevice* dev );


void SetViewport      ( RDICommandQueue* cmdq, RDIViewport vp );
void SetVertexBuffers ( RDICommandQueue* cmdq, RDIVertexBuffer* vbuffers, unsigned start, unsigned n );
void SetIndexBuffer   ( RDICommandQueue* cmdq, RDIIndexBuffer ibuffer );
void SetShaderPass    ( RDICommandQueue* cmdq, RDIShaderPass pass );
void SetInputLayout   ( RDICommandQueue* cmdq, RDIInputLayout ilay );

void SetCbuffers	  ( RDICommandQueue* cmdq, RDIConstantBuffer* cbuffers, unsigned startSlot, unsigned n, unsigned stageMask );
void SetResourcesRO	  ( RDICommandQueue* cmdq, RDIResourceRO* resources, unsigned startSlot, unsigned n, unsigned stageMask );
void SetResourcesRW	  ( RDICommandQueue* cmdq, RDIResourceRW* resources, unsigned startSlot, unsigned n, unsigned stageMask );
void SetSamplers	  ( RDICommandQueue* cmdq, RDISampler* samplers, unsigned startSlot, unsigned n, unsigned stageMask );

void SetDepthState    ( RDICommandQueue* cmdq, RDIDepthState state );
void SetBlendState    ( RDICommandQueue* cmdq, RDIBlendState state );
void SetRasterState   ( RDICommandQueue* cmdq, RDIRasterState state );
void SetHardwareState ( RDICommandQueue* cmdq, RDIHardwareState hwstate );
void SetScissorRects  ( RDICommandQueue* cmdq, const RDIRect* rects, int n );
void SetTopology      ( RDICommandQueue* cmdq, int topology );

void ChangeToMainFramebuffer( RDICommandQueue* cmdq );
void ChangeRenderTargets	( RDICommandQueue* cmdq, RDITextureRW* colorTex, unsigned nColor, RDITextureDepth depthTex = RDITextureDepth(), bool changeViewport = true );

unsigned char* Map	( RDICommandQueue* cmdq, RDIResource resource, int offsetInBytes, RDIEMapType::Enum mapType = RDIEMapType::WRITE );
void           Unmap( RDICommandQueue* cmdq, RDIResource resource );

unsigned char* Map( RDICommandQueue* cmdq, RDIVertexBuffer vbuffer, int firstElement, int numElements, RDIEMapType::Enum mapType );
unsigned char* Map( RDICommandQueue* cmdq, RDIIndexBuffer ibuffer, int firstElement, int numElements, RDIEMapType::Enum mapType );

inline unsigned char* Map( RDICommandQueue* cmdq, RDIVertexBuffer vbuffer, RDIEMapType::Enum mapType ) { return Map( cmdq, vbuffer, 0, vbuffer.numElements, mapType ); }
inline unsigned char* Map( RDICommandQueue* cmdq, RDIIndexBuffer ibuffer , RDIEMapType::Enum mapType ) { return Map( cmdq, ibuffer, 0, ibuffer.numElements, mapType ); }

void UpdateCBuffer( RDICommandQueue* cmdq, RDIConstantBuffer cbuffer, const void* data );
void UpdateTexture( RDICommandQueue* cmdq, RDITextureRW texture, const void* data );

void Draw                ( RDICommandQueue* cmdq, unsigned numVertices, unsigned startIndex );
void DrawIndexed         ( RDICommandQueue* cmdq, unsigned numIndices, unsigned startIndex, unsigned baseVertex );
void DrawInstanced       ( RDICommandQueue* cmdq, unsigned numVertices, unsigned startIndex, unsigned numInstances );
void DrawIndexedInstanced( RDICommandQueue* cmdq, unsigned numIndices, unsigned startIndex, unsigned numInstances, unsigned baseVertex );

void Dispatch            ( RDICommandQueue* cmdq, unsigned numGroupsX, unsigned numGroupsY, unsigned numGroupsZ );

void ClearState       ( RDICommandQueue* cmdq );
void ClearBuffers     ( RDICommandQueue* cmdq, RDITextureRW* colorTex, unsigned nColor, RDITextureDepth depthTex, const float rgbad[5], int flag_color, int flag_depth );
void ClearDepthBuffer ( RDICommandQueue* cmdq, RDITextureDepth depthTex, float clearValue );
void ClearColorBuffers( RDICommandQueue* cmdq, RDITextureRW* colorTex, unsigned nColor, float r, float g, float b, float a );
inline void ClearColorBuffer( RDICommandQueue* cmdq, RDITextureRW colorTex, float r, float g, float b, float a )
{
    ClearColorBuffers( cmdq, &colorTex, 1, r, g, b, a );
}

void Swap( RDICommandQueue* cmdq, unsigned syncInterval = 0 );
void GenerateMipmaps( RDICommandQueue* cmdq, RDITextureRW texture );
RDITextureRW GetBackBufferTexture( RDICommandQueue* cmdq );
void ReportLiveObjects( RDIDevice* dev );


