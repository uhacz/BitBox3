#pragma once

#include <stdint.h>


struct BXIAllocator;

struct BXFileHandle { uint32_t i = 0; };
inline bool IsValid( BXFileHandle h ) { return h.i != 0; }

namespace BXEFileStatus
{
	enum E : int32_t
	{
		EMPTY = 0,
		NOT_FOUND,
		LOADING,
		READY,
	};
}//

namespace BXEFIleMode
{
    enum E : uint32_t
    {
        TXT,
        BIN,
    };
}//

namespace BXEFileListFlag
{
    enum E : uint32_t
    {
        ONLY_NAMES = 1 << 0,
        RECURSE = 1 << 1,
    };
}//

struct BXFile
{
	union
	{
		void* pointer = nullptr;
		uint8_t* bin;
		const char* txt;
	};
	uint32_t size = 0;
	BXIAllocator* allocator = nullptr;
};

struct BXFileWaitResult
{
	BXEFileStatus::E status;
	BXFileHandle handle;
	BXFile file;
};

struct BXIFilesystem;
struct BXPostLoadCallback
{
    typedef void( *CallbackFunction )(BXIFilesystem* filesystem, BXFileHandle hfile, BXEFileStatus::E status, void* user_data0, void* user_data1, void* user_data2 );
    CallbackFunction callback = nullptr;
    void* user_data0 = nullptr;
    void* user_data1 = nullptr;
    void* user_data2 = nullptr;

    BXPostLoadCallback( CallbackFunction cb, void* ud0, void* ud1 = nullptr, void* ud2 = nullptr )
        : callback( cb ), user_data0( ud0 ), user_data1( ud1 ), user_data2( ud2 ) {}

    BXPostLoadCallback() = default;
};

// --- 
struct string_buffer_t;
struct BXIFilesystem
{
	virtual ~BXIFilesystem() {}

	virtual void			 SetRoot  ( const char* absoluteDirPath ) = 0;
    virtual const char*      GetRoot  () const = 0;
	virtual BXFileHandle	 LoadFile ( const char* relativePath, BXEFIleMode::E mode, BXPostLoadCallback callback, BXIAllocator* allocator = nullptr ) = 0;
    virtual BXFileHandle	 LoadFile ( const char* relativePath, BXEFIleMode::E mode, BXIAllocator* allocator = nullptr ) { return LoadFile( relativePath, mode, BXPostLoadCallback{ nullptr,nullptr }, allocator ); }
    virtual void			 CloseFile( BXFileHandle* fhandle, bool freeData = true ) = 0;
	
	virtual BXEFileStatus::E File     ( BXFile* file, BXFileHandle fhandle ) = 0;
};
extern BXIFilesystem* FileSys();
void BXFilesystemStartup( BXIAllocator* allocator );
void BXFilesystemShutdown( BXIAllocator* allocator );

BXFileWaitResult LoadFileSync( const char* relativePath, BXEFIleMode::E mode, BXIAllocator* allocator );
int32_t          WriteFileSync( const char* relativePath, const void* data, uint32_t data_size );
void             ListFiles( string_buffer_t* s, const char* relative_path, uint32_t flags, BXIAllocator* allocator );

