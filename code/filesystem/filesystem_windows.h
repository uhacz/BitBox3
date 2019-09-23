#pragma once

#include "filesystem.h"

#include "../foundation/debug.h"
#include "../foundation/id_table.h"
#include "../foundation/thread/semaphore.h"
#include "../util/file_system_name.h"
#include <thread>
#include <mutex>
#include <atomic>


namespace bx
{
    struct FileInputInfo
	{
		FSName _name;
		BXEFIleMode::E _mode;
        BXPostLoadCallback _callback;
        BXIAllocator* _allocator;
	};

struct FilesystemWindows : BXIFilesystem
{
	FilesystemWindows( BXIAllocator* allocator );

	bool		 Startup();
	void		 Shutdown();
	// --- interface
	bool			 IsValid( BXFileHandle fhandle );
	void			 SetRoot( const char* absoluteDirPath ) override final;
    const char*      GetRoot() const override;
	BXFileHandle	 LoadFile( const char* relativePath, BXEFIleMode::E mode, BXPostLoadCallback callback, BXIAllocator* allocator = nullptr ) override final;
	void			 CloseFile( BXFileHandle* fhandle, bool freeData ) override final;
	BXEFileStatus::E File( BXFile* file, BXFileHandle fhandle ) override final;

	// ---
	static void ThreadProcStatic( FilesystemWindows* fs );
	void ThreadProc();

	// ---

	// --- data
	enum
	{
		MAX_HANDLES = 1024,
		MAX_PATHS = 32,
	};
	using IdManager = id_table_t< MAX_HANDLES >;

	std::thread				_thread;
	light_semaphore_t		_semaphore;
	std::atomic_uint32_t	_is_running = 0;
	
	IdManager     _ids;
	std::mutex    _id_lock;
	
	FileInputInfo		_input_info  [MAX_HANDLES] = {};
	BXFile				_files       [MAX_HANDLES] = {};
	std::atomic_int32_t _files_status[MAX_HANDLES] = {};

	queue_t<BXFileHandle>  _to_load;
	queue_t<BXFile>		   _to_unload;

	std::mutex _to_load_lock;
	std::mutex _to_unload_lock;

	FSName		  _root;
	BXIAllocator* _allocator = nullptr;
};

}// 
