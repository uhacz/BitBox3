#include "resources.h"
#include "containers.h"

#include "../foundation/EASTL/bitset.h"
#include "../foundation/EASTL/array.h"
#include "../foundation/EASTL/hash_map.h"
#include "../foundation/thread/rw_spin_lock.h"
#include "../foundation/hashed_string.h"
#include "../foundation/container_allocator.h"
#include "../foundation/io.h"
#include "../filesystem/filesystem.h"
#include "../util/system_util.h"

#include <new>

//static constexpr u32 MAX_TOKENS = 64;
static constexpr u32 MAX_RESOURCES = 64;


struct RESPathHash 
{
    hashed_string_t hash;

    static RESPathHash Build( const u64 hash ) { return { hash }; }
    static RESPathHash Build( const char* str ) { return { hashed_string( str ) }; }
    static RESPathHash Build( const RESPath& path ) { return { hashed_string( path._path.c_str() ) }; }
};

inline bool operator == ( const RESPathHash& a, const RESPathHash& b ) { return a.hash == b.hash; }
template <> struct eastl::hash<RESPathHash> {
    size_t operator()( RESPathHash val ) const { return val.hash; }
};

struct RESManager::Impl
{
    //eastl::bitset<MAX_TOKENS> token_freemask;
    //eastl::array<u16,MAX_TOKENS> token_generation;
    //eastl::array<RESToken, MAX_TOKENS> token;

    eastl::bitset<MAX_RESOURCES>               resource_freemask;
    eastl::array<u32, MAX_RESOURCES>           resource_generation;
    eastl::array<RESFile*, MAX_RESOURCES>      resource_file;
    eastl::array<RESPath , MAX_RESOURCES>      resource_path;
    eastl::array<RESStatus::E, MAX_RESOURCES > resource_status;
    
    eastl::array<u16, MAX_RESOURCES>        resource_refcount;
    eastl::hash_map<RESPathHash, RESHandle, eastl::hash<RESPathHash>, eastl::equal_to<RESPathHash>, bx_container_allocator> resource_map;

    BXIAllocator* _allocator = nullptr;
    BXIAllocator* file_load_allocator = nullptr;

    mutable rw_spin_lock_t map_lock;
    mutable rw_spin_lock_t token_lock;
    mutable rw_spin_lock_t resource_lock;

    void StartUp( BXIAllocator* allocator ) 
    {
        _allocator = allocator;
        file_load_allocator = _allocator;
    }
    void ShutDown() 
    {
        file_load_allocator = nullptr;
        _allocator = nullptr;
    }

    bool IsAlive_NoLock( RESHandle handle ) const
    {
        const u32 handle_ok = handle.IsValid() && handle.index < MAX_RESOURCES;
        const u32 generation_ok = resource_generation[handle.index] == handle.generation;
        return handle_ok & generation_ok;
    }
    //bool IsAlive_NoLock( RESTokenHandle handle ) const
    //{
    //    const u32 handle_ok = handle.IsValid() && handle.index < MAX_TOKENS;
    //    const u32 generation_ok = token_generation[handle.index] == handle.generation;
    //    return handle_ok & generation_ok;
    //}
};
inline bool IsAlive( const RESManager::Impl* impl, const RESHandle handle )
{
    scoped_read_spin_lock_t guard( impl->resource_lock );
    return impl->IsAlive_NoLock( handle );
}
//inline bool IsAlive( const RESManager::Impl* impl, const RESTokenHandle token )
//{
//    scoped_read_spin_lock_t guard( impl->token_lock );
//    return impl->IsAlive_NoLock( token );
//}


//static RESTokenHandle AllocateToken( RESManager::Impl* impl )
//{
//    RESTokenHandle result;
//
//    {
//        scoped_write_spin_lock_t guard( impl->token_lock );
//        result.index = impl->token_freemask.find_first();
//        SYS_ASSERT( result.index != impl->token_freemask.size() );
//        impl->token_freemask.set( result.index );
//    }
//        
//    result.generation = impl->token_generation[result.index];
//    return result;
//}
//
//static void FreeToken( RESManager::Impl* impl, RESTokenHandle token )
//{
//    if( !token.IsValid() )
//        return;
//
//    scoped_write_spin_lock_t guard( impl->token_lock );
//    if( impl->IsAlive_NoLock( token ) )
//    {
//        impl->token_generation[token.index] += 1;
//        impl->token_freemask.reset( token.index );
//    }
//}

static RESHandle AllocateResource( RESManager::Impl* impl )
{
    RESHandle handle;
    {
        scoped_write_spin_lock_t guard( impl->resource_lock );
        handle.index = impl->resource_freemask.find_first();
        SYS_ASSERT( handle.index != impl->resource_freemask.size() );
        impl->resource_freemask.set( handle.index );
    }

    handle.generation = impl->resource_generation[handle.index];
    return handle;
}
static void FreeResource( RESManager::Impl* impl, RESHandle handle )
{
    if( !handle.IsValid() )
        return;

    scoped_write_spin_lock_t guard( impl->token_lock );
    if( impl->IsAlive_NoLock( handle ) )
    {
        impl->resource_generation[handle.index] += 1;
        impl->resource_freemask.reset( handle.index );
    }
}

RESHandle RESManager::IssueLoading( const RESPath& path, LoadedCallback&& callback )
{
    const RESPathHash path_hash = RESPathHash::Build( path );
    RESHandle handle;
    {
        scoped_read_spin_lock_t guard( impl->map_lock );
        const auto it = impl->resource_map.find( path_hash );
        if( it != impl->resource_map.end() )
        {
            handle = it->second;
        }
    }

    {
        scoped_write_spin_lock_t guard( impl->resource_lock );
        if( impl->IsAlive_NoLock( handle ) )
        {
            SYS_ASSERT( !impl->resource_freemask.test( handle.index ) );
            impl->resource_refcount[handle.index] += 1;
        }
        else
        {
            handle = {};
        }
    }

    if( handle.IsValid() )
    {
        // we have found a resource and refcount has been incremented
        //RESTokenHandle htok = AllocateToken( impl );
        //RESToken& tok = impl->token[htok.index];
        //tok.result = RESToken::eSUCCESS;
        //tok.self_id = htok;
        //tok.resource_handle = handle;
        //tok.wait_counter = {};
        //if( callback )
        //{
        //    callback( tok );
        //}
        //return htok;
        if( callback )
        {
            callback( handle, RESStatus::eSUCCESS );
        }
        return handle;

    }
    else
    {
        {
            scoped_write_spin_lock_t guard( impl->map_lock );
            SYS_ASSERT( impl->resource_map.find( path_hash ) == impl->resource_map.end() );
            auto it = impl->resource_map.insert( path_hash );
            impl->resource_map[path_hash] = handle;

            SYS_ASSERT( impl->resource_refcount[handle.index] == 0 );
            impl->resource_refcount[handle.index] = 2;
        }

        // we have to load resource
        //RESTokenHandle htok = AllocateToken( impl );
        RESHandle handle = AllocateResource( impl );
        
        SYS_ASSERT( handle.IsValid() );
        //SYS_ASSERT( htok.IsValid() );

        impl->resource_path[handle.index] = path;
        impl->resource_status[handle.index] = RESStatus::eLOADING;
        JOB::Create( "ResourceLoading", [this, handle, cb = std::move( callback )]( const JOBRange range, const JOBContext& )
        {
            SYS_ASSERT( IsAlive( impl, handle ) );
            SYS_ASSERT( impl->resource_status[handle.index] == RESStatus::eLOADING );
            SYS_ASSERT( impl->resource_file[handle.index] == nullptr );
            SYS_ASSERT( impl->resource_refcount[handle.index] >= 1 );

            const RESPath& path = impl->resource_path[handle.index];

            const char* root_path = FileSys()->GetRoot();
            char abs_path[512];
            snprintf( abs_path, 512, "%s/%s", root_path, path._path.c_str() );

            u8* file_buffer = nullptr;
            u32 file_size = 0;
            const i32 load_error = ReadFile( &file_buffer, &file_size, abs_path, impl->file_load_allocator );
            RESStatus::E status = RESStatus::eEMPTY;
            if( load_error == 0 )
            {
                RESFile* file_data = (RESFile*)file_buffer;
                if( file_data->system_tag != RESFile::SYSTEM_TAG )
                {
                    status = RESStatus::eFAIL_NOT_RESOURCE;
                }
                else
                {
                    scoped_read_spin_lock_t guard( impl->resource_lock );
                    impl->resource_file[handle.index] = file_data;
                    status = RESStatus::eSUCCESS;
                }
            }
            else
            {
                status = RESStatus::eFAIL_FILE_NOT_FOUND;
            }

            impl->resource_status[handle.index] = status;
            if( status == RESStatus::eFAIL_NOT_RESOURCE )
            {
                BX_FREE( impl->file_load_allocator, file_buffer );
            }

            if( cb )
            {
                cb( handle, status );
            }
        } );


        //RESToken tok = impl->token[htok.index];
        //tok.self_id = htok;
        //tok.resource_handle = handle;
        //tok.result = RESToken::eLOADING;
        //tok.wait_counter = 

        return handle;
    }
}

RESHandle RESManager::IssueLoading( const RESPath& path )
{
    return {};
}

void RESManager::Unload( const RESHandle& handle )
{

}

const RESFile* RESManager::Data( RESHandle handle ) const
{
    scoped_read_spin_lock_t guard( impl->resource_lock );
    return (impl->IsAlive_NoLock( handle )) ? impl->resource_file[handle.index] : nullptr;
}

//
//RESToken RESManager::Token( const RESTokenHandle& tokenid )
//{
//    return {};
//}

//RESHandle RESManager::ReleaseToken( RESTokenHandle tokenid )
//{
//    return RESHandle();
//}

static RESManager* __resources = nullptr;
void RESManager::StartUp( BXIAllocator* allocator )
{
    __resources = System_StartUp<RESManager, RESManager::Impl>( allocator );
}

void RESManager::ShutDown()
{
    System_ShutDown<RESManager, RESManager::Impl>( &__resources );
}


extern RESManager* gResources()
{
    return __resources;
}
