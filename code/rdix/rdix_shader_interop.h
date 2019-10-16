#pragma once

#include <foundation/math/vmath_type.h>
#include <foundation/type_compound.h>
#include <rdi_backend/rdi_backend_type.h>

namespace shader
{
    using float2 = vec2_t;
    using float3 = vec3_t;
    using float4 = vec4_t;
    using float3x3 = mat33_t;
    using float4x4 = mat44_t;
    using uint = uint32_t;
    using uint2 = u32x2;
    using uint3 = u32x3;
    using uint4 = u32x4;

#include <shaders/hlsl/samplers.h>
#include <shaders/hlsl/material_data.h>
#include <shaders/hlsl/frame_data.h>
#include <shaders/hlsl/transform_instance_data.h>
#include <shaders/hlsl/skinning.h>
#include <shaders/hlsl/vertex_layout.h>
}//
