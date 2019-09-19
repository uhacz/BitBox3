#include "shader_compiler.h"
#include "hardware_state_util.h"
#include "shader_file_writer.h"
#include "shader_compiler_dx11.h"

namespace bx{ namespace tool{
int ShaderCompilerCompile( const char* inputFile, const char* outputDir, BXIAllocator* allocator )
{
    ShaderFileWriter writer;
    int ires = writer.StartUp( inputFile, outputDir, allocator );

    print_info( "%s...\n", inputFile );

    if( ires == 0 )
    {
        dx11Compiler compiler( allocator );

        SourceShader source_shader;
        CompiledShader compiled_shader;
        
        File inputFile = writer.InputFile();

        ires = ParseHeader( &source_shader, (const char*)inputFile.data, (int)inputFile.size );
        if( ires == 0 )
        {
            ires = compiler.Compile( &compiled_shader, source_shader, (const char*)inputFile.data, writer.InputDir(), 1 );
        }

        if( ires == 0 )
        {
            DataBlob shader_blob = CreateShaderBlob( compiled_shader, allocator );
            writer.WriteBinary( shader_blob.ptr, shader_blob.size );
            Release( &shader_blob );

            for( const BinaryPass& bpass : compiled_shader.passes )
            {
                for( uint32_t i = 0; i < RDIEPipeline::COUNT; ++i )
                {
                    if( !bpass.disassembly[i].ptr )
                        continue;

                    const char* stageName = RDIEPipeline::name[i];
                    writer.WtiteAssembly( bpass.name.c_str(), stageName, bpass.disassembly[i].ptr, bpass.disassembly[i].size );
                }
            }
        }

        Release( &compiled_shader, allocator );
        Release( &source_shader );
    }

    writer.ShutDown();

    return ires;
    
    return 0;
}

}}///
