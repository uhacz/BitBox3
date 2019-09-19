#include "shader_file_writer.h"
#include <memory/memory.h>
#include <iostream>


namespace bx{ namespace tool{

	static inline void CheckSlashes( std::string* str )
	{
		for( std::string::iterator it = str->begin(); it != str->end(); ++it )
		{
			if( *it == '\\' )
				*it = '/';
		}
	}

int ShaderFileWriter::StartUp( const char* in_file_path, const char* output_dir, BXIAllocator* allocator )
{
	_allocator = allocator;
    int ires = 0;

    ires = _ParseFilePath( in_file_path );
    if( ires < 0 )
        return ires;

    _out_filename = _in_filename;
    _out_filename.erase( _out_filename.find_last_of( '.' ) );
	_out_dir = output_dir;
	CheckSlashes( &_out_dir );

	if( _out_dir.back() != '/' )
		_out_dir.append( "/" );

	int32_t read_result = ReadTextFile( &_in_file.data, &_in_file.size, in_file_path, allocator );
	if( read_result == IO_ERROR )
	{
		std::cerr << "cannot read file: " << _in_filename << std::endl;
		return -1;
	}

	

    return 0;
}

void ShaderFileWriter::ShutDown()
{
	BX_FREE0( _allocator, _in_file.data );
	_in_file.size = 0;
}

int ShaderFileWriter::WriteBinary( const void* data, size_t dataSize )
{
	std::string out_file_path = _out_dir + "bin";
	CreateDir( out_file_path.c_str() );

    out_file_path.append( "/" );
    out_file_path.append( _out_filename );
    out_file_path.append( ".shader" );

    return WriteFile( out_file_path.c_str(), data, dataSize );
}

int ShaderFileWriter::WtiteAssembly( const char* passName, const char* stageName, const void* data, size_t dataSize )
{
	std::string out_file_path = _out_dir + "asm";
	CreateDir( out_file_path.c_str() );

	out_file_path.append( "/" );
	out_file_path.append( _out_filename );
    out_file_path.append( "." );
    out_file_path.append( passName );
    out_file_path.append( "." );
    out_file_path.append( stageName );
	out_file_path.append( ".asm" );

    return WriteFile( out_file_path.c_str(), data, dataSize );
}

//////////////////////////////////////////////////////////////////////////
int ShaderFileWriter::_ParseFilePath( const char* file_path )
{
    std::string file_str( file_path );
	CheckSlashes( &file_str );

    const size_t slash_pos = file_str.find_last_of( '/' );
    if( slash_pos == std::string::npos )
    {
        std::cerr << "input path not valid\n";
        return -1;
    }

    std::string root_dir = file_str.substr( 0, slash_pos + 1 );
    std::string file_name = file_str.substr( slash_pos + 1 );

	_in_dir = root_dir;
	_in_filename = file_name;

    return 0;
}

}
}///
