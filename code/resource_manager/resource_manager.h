#pragma once

#include <foundation/type.h>
#include "resource_loader.h"

struct BXIFilesystem;
struct BXIAllocator;

namespace RSMEState
{
    enum E : uint32_t
    {
        UNLOADED = 0,
        LOADING,
        READY,
        UNLOADING,
        FAIL,
    };
}//

struct RSMResourceHash
{
    uint64_t h;
};
struct RSMResourceID
{
    uint32_t i;

    static constexpr RSMResourceID Null() { return { 0 }; }
};

namespace RSM
{
    RSMResourceHash CreateHash( const char* relative_path );

    RSMResourceID Load( const char* relative_path, void* system = nullptr );
    RSMResourceID Create( const char* name, const void* data );
    RSMResourceID Create( const void* data );
    RSMEState::E  Wait( RSMResourceID rid );

    RSMResourceID Find( const char* relative_path );
    RSMResourceID Find( RSMResourceHash hash );
    bool IsAlive( RSMResourceID id );

    RSMEState::E State( RSMResourceID id );
    const void* Get( RSMResourceID id );
    
    // returns true when ref count has reached zero 
    bool Release( RSMResourceID id );
    bool Release( RSMResourceID id, void** resource_pointer );

    inline const void* Acquire( const char* relative_path )
    {
        RSMResourceID rid = Find( relative_path );
        return IsAlive( rid ) ? Get( rid ) : nullptr;
    }

    void Acquire( RSMResourceID id );

   
    template<typename T>
    inline void RegisterLoader() { Internal_AddLoader( T::Internal_Creator ); }
    
    // --- private

    BXIFilesystem* Filesystem();
    void Internal_AddLoader( RSMLoaderCreator* creator );
    
    void StartUp( BXIFilesystem* filesystem, BXIAllocator* allocator );
    void ShutDown( );
}//

struct RSMLoadState
{
    uint32_t nb_loaded = 0;
    uint32_t nb_failed = 0;
};
template< typename T >
RSMLoadState GetLoadState( T** resources, const RSMResourceID* ids, uint32_t count )
{
    RSMLoadState result = {};
    for( uint32_t i = 0; i < count; ++i )
    {
        RSMEState::E state = RSM::State( ids[i] );
        if( state == RSMEState::READY )
            result.nb_loaded += 1;
        else if( state == RSMEState::FAIL )
            result.nb_failed += 1;
    }

    if( resources && (count == result.nb_loaded) )
    {
        for( uint32_t i = 0; i < count; ++i )
        {
            resources[i] = (T*)RSM::Get( ids[i] );
            SYS_ASSERT( resources[i] != nullptr );
        }
    }

    return result;
}
