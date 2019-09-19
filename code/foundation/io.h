#pragma once


enum EIOResult : int
{
	IO_OK = 0,
	IO_ERROR = -1,
};

struct BXIAllocator;
int ReadFile    ( unsigned char** outBuffer, unsigned* outSizeInBytes, const char* path, BXIAllocator* allocator );
int ReadTextFile( unsigned char** outBuffer, unsigned* outSizeInBytes, const char* path, BXIAllocator* allocator );
int WriteFile   ( const char* absPath, const void* buf, size_t sizeInBytes );
int CopyFile    ( const char* absDstPath, const char* absSrcPath, BXIAllocator* allocator );
int CreateDir   ( const char* absPath );