#pragma once

#include <foundation/math/vmath_type.h>
#include <foundation/type.h>

struct RDIXDebugParams
{
    uint32_t color = 0xFFFFFFFF;
    uint32_t use_depth : 1;
    uint32_t is_solid : 1;
    float scale = 1.f;

    RDIXDebugParams( uint32_t c = 0xFFFFFFFF ) : color( c ) { use_depth = 1; is_solid = 0; }
    RDIXDebugParams& NoDepth()  { use_depth = 0; return *this; }
    RDIXDebugParams& Solid() { is_solid = 1; return *this; }
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

    void Flush( RDICommandQueue* cmdq, const mat44_t& viewproj );
}//
