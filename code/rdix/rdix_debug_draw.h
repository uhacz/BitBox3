#pragma once

#include <foundation/math/vmath_type.h>
#include <foundation/type.h>
#include "rdi_backend/rdi_backend_type.h"

struct RDIXDebugParams
{
    uint32_t color = 0xFFFFFFFF;
    RDIHardwareStateDesc hw_state;
    float scale = 1.f;

    RDIXDebugParams( uint32_t c = 0xFFFFFFFF ) : color( c ) { hw_state.raster.FillMode( RDIEFillMode::WIREFRAME ); }
    RDIXDebugParams& NoDepth()  { hw_state.depth.Test(0).Write(0); return *this; }
    RDIXDebugParams& Solid()    { hw_state.raster.FillMode( RDIEFillMode::SOLID ); return *this; }
    RDIXDebugParams& Scale( float v ) { scale = v; return *this; }
};

struct BXIAllocator;
struct RDIDevice;
struct RDICommandQueue;

namespace RDIXDebug
{
    void StartUp( RDIDevice* dev, BXIAllocator* allocator );
    void ShutDown( RDIDevice* dev );
    
    void AddAABB( const vec3_t& center, const vec3_t& extents, const RDIXDebugParams& params = {} );
    void AddSphere( const vec3_t& pos, float radius, const RDIXDebugParams& params = {} );
    void AddLine( const vec3_t& start, const vec3_t& end, const RDIXDebugParams& params = {} );
    void AddAxes( const mat44_t& pose, const RDIXDebugParams& params = {} );

    void Flush( RDIDevice* dev, RDICommandQueue* cmdq, const mat44_t& viewproj );
}//
