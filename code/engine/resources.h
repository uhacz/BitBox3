#pragma once

#include "../foundation/type.h"
#include "../foundation/string_util.h"

#include "../job/job.h"
#include "functional"
#include "tag.h"

struct BXIAllocator;
struct BXIFilesystem;

struct RESPath
{
    string_t _path;
};


namespace RESStatus
{
    enum E : u8
    {
        eEMPTY,
        eFAIL_FILE_NOT_FOUND,
        eFAIL_NOT_RESOURCE,
        eSUCCESS,
        eLOADING,
    };
}

template< typename T, u32 IndexBits>
union RESHandleBase
{
    static constexpr u32 NbBits = sizeof( T ) * 8;
    static_assert( NbBits > IndexBits, "IndexBits must not be greater or equal than total bits available" );
    T hash = 0;
    struct
    {
        T index : IndexBits;
        T generation : NbBits - IndexBits;
    };

    bool IsValid() const { return generation != 0; }
};

using RESHandle = RESHandleBase<u32, 16>;
//using RESTokenHandle = RESHandleBase<u32, 15>;

struct RESFile
{
    static constexpr u32 SYSTEM_TAG = BX_UTIL_TAG32( 'R','E','S','F' );
    const u32 system_tag = SYSTEM_TAG;
    u32 tag = 0;
    u32 version = 0;
    u32 type_signature = 0;
    u32 payload_offset = 0;

    template< typename T > T* Payload()
    {
        SYS_ASSERT( type_signature == T::type_signature );
        return version == T::version ? TYPE_OFFSET_GET_POINTER( T, payload_offset );
    }
    template< typename T > const T* Payload() const
    {
        SYS_ASSERT( type_signature == T::type_signature );
        return version == T::version ? TYPE_OFFSET_GET_POINTER( T, payload_offset );
    }
};



//struct RESToken
//{
//    enum Result : u8
//    {
//        eFAIL,
//        eSUCCESS,
//        eLOADING,
//    };
//    Result result;
//    RESTokenHandle self_id;
//    RESHandle resource_handle;
//    JOBTaskID wait_counter;
//};

struct RESManager
{
    using LoadedCallback = std::function<void( const RESHandle handle, RESStatus::E status )>;
    RESHandle IssueLoading( const RESPath& path, LoadedCallback&& callback );
    RESHandle IssueLoading( const RESPath& path );
    void Unload( const RESHandle& handle );

    const RESFile* Data( RESHandle handle ) const;

    //RESToken Token( const RESTokenHandle& tokenid );
    //RESHandle ReleaseToken( RESHandle tokenid );

    // private
    struct Impl;
    Impl* impl;
    static void StartUp( BXIAllocator* allocator );
    static void ShutDown();
};

extern RESManager* gResources();
