#include "shader_compiler.h"
#include <memory/memory.h>
#include <3rd_party/AnyOption/anyoption.h>
#include "memory/memory.h"

#define TEST 0

int main( int argc, char** argv )
{
	BXMemoryStartUp();
    BXIAllocator* allocator = BXDefaultAllocator();

#if TEST == 1
    const char input_file[] = "x:/dev/BitBox_v2/code/shaders/hlsl/skinning.hlsl";
	const char output_dir[] = "x:/dev/assets/shader/hlsl/";
#else
    AnyOption opt;
    opt.addUsage( "" );
    opt.addUsage( "Usage:" );
    opt.addUsage( "" );
    opt.addUsage( "--input-file     input file (absolute path)" );
    opt.addUsage( "--output-dir     output directory (absolute path)" );

    opt.setOption( "input-file" );
    opt.setOption( "output-dir" );

    opt.processCommandArgs( argc, argv );
    if( !opt.hasOptions() )
    {
        opt.printUsage();
        return -1;
    }
    
    const char* input_file = opt.getValue( "input-file" );
    const char* output_dir = opt.getValue( "output-dir" );

    if( !input_file || !output_dir )
    {
        opt.printUsage();
        return -2;
    }

#endif

	bx::tool::ShaderCompilerCompile( input_file, output_dir, allocator );
	
	BXMemoryShutDown();
	return 0;
}
