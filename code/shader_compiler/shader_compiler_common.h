#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <map>

#include <foundation/io.h>
#include <memory/memory.h>
//#include <foundation/buffer_utils.h>
#include <foundation/hash.h>


#include <rdi_backend/rdi_backend_type.h>
#include <rdi_backend/rdi_shader_reflection.h>

#include <rdix/rdix_type.h>

namespace bx{
using namespace rdi;

namespace tool{

	struct File
	{
		uint8_t* data = nullptr;
		uint32_t size = 0;
		BXIAllocator* allocator = nullptr;
	};

    struct ConfigPass
    {
        struct MacroDefine
        {
            const char* name = nullptr;
            const char* def = nullptr;
        };
        const char* name = nullptr;
        const char* entry_points[RDIEPipeline::COUNT] = {};
        const char* versions[RDIEPipeline::COUNT] = {};

        RDIHardwareStateDesc hwstate = {};
        MacroDefine defs[cRDI_MAX_SHADER_MACRO + 1] = {};
    };

    struct DataBlob
    {
        void*   ptr = nullptr;
        size_t  size = 0;
        void* __priv = nullptr;
		BXIAllocator* _allocator = nullptr;
    };

    struct BinaryPass
    {
        std::string name;
        RDIHardwareStateDesc hwstate_desc = {};
        RDIXResourceBinding* rdesc = nullptr;
        uint32_t rdesc_mem_size = 0;
        
        DataBlob bytecode[RDIEPipeline::COUNT] = {};
        DataBlob disassembly[RDIEPipeline::COUNT] = {};

        RDIShaderReflection reflection = {};
    };

    struct SourceShader
    {
		void* _internal_data = nullptr;
        std::vector< ConfigPass > passes;
    };

    struct CompiledShader
    {
        std::vector< BinaryPass > passes;
    };

    //////////////////////////////////////////////////////////////////////////
    ///
    void Release( DataBlob* blob );
    void Release( SourceShader* fx_source );
    void Release( CompiledShader* fx_bin, BXIAllocator* allocator );
    

    //////////////////////////////////////////////////////////////////////////
    ///
    int ParseHeader( SourceShader* fx_src, const char* source, int source_size );
    DataBlob CreateShaderBlob( const CompiledShader& compiled, BXIAllocator* allocator );


#define print_error( str, ... ) do{ fprintf( stdout, str, __VA_ARGS__ ); fflush( stdout ); }while(0)
#define print_info( str, ... ) do{ fprintf( stdout, str, __VA_ARGS__ ); fflush( stdout ); }while(0)

}}///
