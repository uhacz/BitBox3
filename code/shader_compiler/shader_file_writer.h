#pragma once

#include <foundation/io.h>
#include <string>
#include "shader_compiler_common.h"


namespace bx{ namespace tool{

class ShaderFileWriter
{
public:
    int StartUp( const char* in_file_path, const char* output_dir, BXIAllocator* allocator );
    void ShutDown();

    int WriteBinary( const void* data, size_t dataSize );
    int WtiteAssembly( const char* passName, const char* stageName, const void* data, size_t dataSize );

    File InputFile() { return _in_file; }

	const char* InputDir() const { return _in_dir.c_str(); }

private:
    int _ParseFilePath( const char* file_path );

private:
	BXIAllocator* _allocator = nullptr;

    std::string _in_filename;
    std::string _out_filename;
	
	std::string _in_dir;
	std::string _out_dir;

    File _in_file;
};

}}///
