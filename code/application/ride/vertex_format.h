#pragma once
#include "../../foundation/math/vmath_type.h"
#include "../../rdi_backend/rdi_backend_type.h"
#include "../../rdix/rdix_shader_interop.h"
namespace shader
{
#include <shaders/hlsl/vertex_layout.h>
}//

struct VertexPN
{
    vec3_t pos;
    vec3_t nrm;

    static void SetAttribs( shader::VertexStream* stream )
    {
        shader::AddAttrib( stream, RDIFormat::Float3() );
        shader::AddAttrib( stream, RDIFormat::Float3() );
    }
};
struct VertexP
{
    vec3_t pos;
    static void SetAttribs( shader::VertexStream* stream )
    {
        shader::AddAttrib( stream, RDIFormat::Float3() );
    }
};

struct VertexPNUV
{
    vec3_t pos;
    vec3_t nrm;
    vec2_t uv;
    static void SetAttribs( shader::VertexStream* stream )
    {
        shader::AddAttrib( stream, RDIFormat::Float3() );
        shader::AddAttrib( stream, RDIFormat::Float3() );
        shader::AddAttrib( stream, RDIFormat::Float2() );
    }
};