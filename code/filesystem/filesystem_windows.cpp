#include "filesystem_windows.h"

#include <memory/memory.h>
#include <foundation/debug.h>
#include <foundation/queue.h>
#include <foundation/io.h>

namespace bx
{

// ---
//


// ---
//
FilesystemWindows::FilesystemWindows( BXIAllocator* allocator )
	: _to_load( allocator )
	, _to_unload( allocator )
	, _allocator( allocator )
{
}
bool FilesystemWindows::Startup()
{
	_is_running = 1;
	_thread = std::thread( ThreadProcStatic, this );

	return true;
}
void FilesystemWindows::Shutdown()
{
	_is_running = 0;
	_semaphore.signal();
	_thread.join();
}
void FilesystemWindows::SetRoot( const char * absoluteDirPath )
{
	_root.Clear();
	bool bres = _root.Append( absoluteDirPath );
	SYS_ASSERT( bres );
}

const char* FilesystemWindows::GetRoot() const
{
    return _root.AbsolutePath();
}

bool FilesystemWindows::IsValid( BXFileHandle fhandle )
{
	const id_t id = { fhandle.i };
	return id_table::has( _ids, id );
}

BXFileHandle FilesystemWindows::LoadFile( const char* relativePath, BXEFIleMode::E mode, BXPostLoadCallback callback, BXIAllocator* allocator )
{
	if( !allocator )
		allocator = _allocator;

	id_t id = { 0 };
	
	_id_lock.lock();
	id = id_table::create( _ids );
	_id_lock.unlock();

	_files_status[id.index].store( BXEFileStatus::LOADING );

	FileInputInfo& info = _input_info[id.index];
	info._mode = mode;
	info._name.Clear();
	info._name.AppendRelativePath( relativePath );
    info._callback = callback;
	info._allocator = allocator;

	BXFileHandle fhandle;
	fhandle.i = id.hash;

	_to_load_lock.lock();
	queue::push_back( _to_load, fhandle );
	_to_load_lock.unlock();
	_semaphore.signal();

	return fhandle;
}

void FilesystemWindows::CloseFile( BXFileHandle* fhandle, bool freeData )
{
	if( !IsValid( *fhandle ) )
		return;

	const id_t id = { fhandle->i };
	_files_status[id.index].store( BXEFileStatus::EMPTY );

	BXFile file = _files[id.index];

	_id_lock.lock();
	id_table::destroy( _ids, id );
	_id_lock.unlock();

	if( freeData )
	{
		_to_unload_lock.lock();
		queue::push_back( _to_unload, file );
		_to_unload_lock.unlock();
		_semaphore.signal();
	}

    _files[id.index] = {};
    fhandle[0] = {};
}

BXEFileStatus::E FilesystemWindows::File( BXFile* file, BXFileHandle fhandle )
{
	if( !IsValid( fhandle ) )
		return BXEFileStatus::EMPTY;

	const id_t id = { fhandle.i };

	BXEFileStatus::E status = (BXEFileStatus::E)_files_status[id.index].load();
	if( status == BXEFileStatus::READY )
		file[0] = _files[id.index];

	return status;
}

void FilesystemWindows::ThreadProcStatic( FilesystemWindows* fs )
{
	fs->ThreadProc();
}

template< typename T>
static inline bool PopFromQueueSafe( T* fhandle, queue_t<T>& queue, std::mutex& lock )
{
	bool result = false;
	lock.lock();
	if( result = !queue::empty( queue ) )
	{
		fhandle[0] = queue::front( queue );
		queue::pop_front( queue );
	}
	lock.unlock();

	return result;
}

void FilesystemWindows::ThreadProc()
{
	while( _is_running )
	{
		_semaphore.wait();

		// load files
		while( true )
		{
			BXFileHandle fhandle = {};
			if( !PopFromQueueSafe( &fhandle, _to_load, _to_load_lock ) )
				break;
				
			const id_t id = { fhandle.i };
			const FileInputInfo& info = _input_info[id.index];

			FSName path;
			path.Append( _root.AbsolutePath() );
			if( path.AppendRelativePath( info._name.AbsolutePath() ) )
			{
				BXFile& file = _files[id.index];
				int32_t result = IO_ERROR;
				
				file.allocator = info._allocator;

				if( info._mode == BXEFIleMode::BIN )
					result = ReadFile( &file.bin, &file.size, path.AbsolutePath(), info._allocator );
				else if( info._mode == BXEFIleMode::TXT )
					result = ReadTextFile( &file.bin, &file.size, path.AbsolutePath(), info._allocator );

				const BXEFileStatus::E file_status = (result == IO_OK) ? BXEFileStatus::READY : BXEFileStatus::NOT_FOUND;
				_files_status[id.index].store( file_status );
                if( info._callback.callback )
                {
                    (*info._callback.callback)(this, fhandle, file_status, info._callback.user_data0, info._callback.user_data1, info._callback.user_data2 );
                }
			}
			else
			{
				SYS_LOG_ERROR( "Filesystem: path '%s' is to long", info._name.AbsolutePath() );
				CloseFile( &fhandle, false );
			}
		}

		// close files
		while( true )
		{
			BXFile file = {};
			if( !PopFromQueueSafe( &file, _to_unload, _to_unload_lock ) )
				break;

			BX_FREE( file.allocator, file.pointer );

		}
	}
}

}//

