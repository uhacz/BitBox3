#pragma once

#include <foundation/type.h>
#include <string>
#include <vector>

namespace bx{ namespace rdi{

struct ShaderVariableDesc
{
    std::string name;
    uint32_t hashed_name = 0;
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t type = 0;
};

struct ShaderBufferDesc
{
    std::string name;
    uint32_t hashed_name = 0;
    uint32_t size = 0;
    uint8_t slot = 0;
    uint8_t stage_mask = 0;
    union
    {
        uint16_t all = 0;
        struct  
        {
            uint16_t cbuffer : 1;
            uint16_t structured : 1;
            uint16_t byteaddress : 1;
            uint16_t uav : 1;
            uint16_t read_write : 1;
            uint16_t append : 1;
            uint16_t consume : 1;
        };
    } flags;
    //std::vector<ShaderVariableDesc> variables;
};

struct ShaderTextureDesc
{
    ShaderTextureDesc() : hashed_name( 0 ), slot( 0 ), dimm( 0 ), is_cubemap( 0 ), pass_index( -1 ) {}
    std::string name;
    uint32_t hashed_name = 0;
    uint8_t slot = 0;
    uint8_t stage_mask = 0;
    uint8_t dimm = 0;
    uint8_t is_cubemap = 0;
    int8_t pass_index = -1;
};

struct ShaderSamplerDesc
{
    std::string name;
    uint32_t hashed_name = 0;
    uint8_t slot = 0;
    uint8_t stage_mask = 0;
    int8_t pass_index = -1;
};

}}///


struct RDIShaderReflection
{
	RDIShaderReflection() {}

	std::vector<bx::rdi::ShaderBufferDesc> buffers;
	std::vector<bx::rdi::ShaderTextureDesc> textures;
	std::vector<bx::rdi::ShaderSamplerDesc> samplers;
	RDIVertexLayout vertex_layout;
	uint16_t input_mask;
	uint16_t __pad0[1];
};
