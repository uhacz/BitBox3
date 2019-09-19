#include "io.h"

#include <stdio.h>
#include <string.h>

#include "type.h"
#include "debug.h"
#include "memory\memory.h"

#include <direct.h>

static FILE* OpenFile( const char* path, const char* mode )
{
	if( !path || strlen( path ) == 0 )
	{
		SYS_LOG_ERROR( "Provided path is empty!\n" );
		return 0;
	}

	FILE* f = nullptr;
	errno_t err = fopen_s( &f, path, mode );
	if( err != 0 )
	{
		SYS_LOG_ERROR( "Can not open file %s (mode: %s | errno: %d)\n", path, mode, err );
	}

	return f;
}
int32_t ReadFile( uint8_t** outBuffer, uint32_t* outSizeInBytes, const char* path, BXIAllocator* allocator )
{
	FILE* f = OpenFile( path, "rb" );
	if( !f )
	{
		return IO_ERROR;
	}

	fseek( f, 0, SEEK_END );
	uint32_t sizeInBytes = (uint32_t)ftell( f );
	fseek( f, 0, SEEK_SET );

	uint8_t* buf = (uint8_t*)BX_MALLOC( allocator, sizeInBytes, 1 );
	SYS_ASSERT( buf && "out of memory?" );
	size_t readBytes = fread( buf, 1, sizeInBytes, f );
	SYS_ASSERT( readBytes == sizeInBytes );
	int ret = fclose( f );
	SYS_ASSERT( ret != EOF );

	*outBuffer = buf;
	*outSizeInBytes = sizeInBytes;

	return IO_OK;
}
int32_t ReadTextFile( uint8_t** outBuffer, uint32_t* outSizeInBytes, const char* path, BXIAllocator* allocator )
{
	FILE* f = OpenFile( path, "rb" );
	if( !f )
	{
		return IO_ERROR;
	}

	fseek( f, 0, SEEK_END );
	uint32_t sizeInBytes = (uint32_t)ftell( f );
	fseek( f, 0, SEEK_SET );

	uint8_t* buf = (uint8_t*)BX_MALLOC( allocator, sizeInBytes + 1, 1 );
	SYS_ASSERT( buf && "out of memory?" );
	size_t readBytes = fread( buf, 1, sizeInBytes, f );
	SYS_ASSERT( readBytes == sizeInBytes );
	int ret = fclose( f );
	SYS_ASSERT( ret != EOF );

	buf[sizeInBytes] = 0;

	*outBuffer = buf;
	*outSizeInBytes = sizeInBytes + 1;

	return IO_OK;
}
int32_t WriteFile( const char* absPath, const void* buf, size_t sizeInBytes )
{
	FILE* fp = OpenFile( absPath, "wb" );
	if( !fp )
	{
		return -1;
	}

	size_t written = fwrite( buf, 1, sizeInBytes, fp );
	fclose( fp );
	if( written != sizeInBytes )
	{
		SYS_LOG_ERROR( "Can't write requested number of bytes to file '%s'\n", absPath );
		return -1;
	}

	return (int)written;
}
int32_t CopyFile( const char* absDstPath, const char* absSrcPath, BXIAllocator* allocator )
{
	uint8_t* buf = 0;
	uint32_t len = 0;
	int32_t res = ReadFile( &buf, &len, absSrcPath, allocator );
	if( res == IO_OK )
	{
		WriteFile( absDstPath, buf, len );
	}

	BX_FREE0( allocator, buf );
	return res;
}
int32_t CreateDir( const char* absPath )
{
	const int res = _mkdir( absPath );
	return (res == ENOENT) ? IO_ERROR : IO_OK;
}