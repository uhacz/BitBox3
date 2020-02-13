#pragma once

#include "../foundation/type.h"
#include "../foundation/common.h"

#include <functional>

struct JOBTaskID
{
    u64 impl;
};

struct JOBContext
{
    JOBTaskID this_job;
    u32 thread_index;
};

struct JOBRange
{
    u32 begin;
    u32 end;
    u32 Count() const { return end - begin; }
};

struct JOBSplit
{
    u32 size;
    u32 chunk;

    static JOBSplit Single() { return { 1, 1 }; }
    static JOBSplit Chunk( u32 s, u32 c ) { return { s,c }; }
    static JOBSplit Worker( u32 s, u32 w )
    {
        const u32 chunk = iceil( s, w );
        return Chunk( s, chunk );
    }
};

using JOBFunc = std::function<void( const JOBRange range, const JOBContext& ) > ;

enum class JOBAutoSpawn : u8
{ YES, NO };

enum class JOBPriority : u8
{
    IMMEDIATE = 0,
    HIGH,
    MEDUIM,
    LOW,
};

struct JOBDesc
{
    JOBAutoSpawn auto_spawn = JOBAutoSpawn::YES;
    JOBPriority priority = JOBPriority::HIGH;
    JOBSplit split = JOBSplit::Single();


    JOBDesc& Split( const JOBSplit& s ) { split = s; return *this; }
    JOBDesc& NoSpawn()   { auto_spawn = JOBAutoSpawn::NO; return *this; }
    JOBDesc& Immediate() { priority = JOBPriority::IMMEDIATE; return *this; }
    JOBDesc& Medium()    { priority = JOBPriority::HIGH; return *this; }
    JOBDesc& Low()       { priority = JOBPriority::LOW; return *this; }
};

struct JOB
{
    static void StartUp( u32 nbThreads = 0 );
    static void ShutDown();
    static u32 GetThreadCount();
    
    //static JOBTaskID Dispatch( const char* name, JOBFunc&& func, const JOBDesc& desc = JOBDesc() );
    //static JOBTaskID Continuation( JOBTaskID parent, const char* name, JOBFunc&& func, const JOBDesc& desc = JOBDesc() );

    static JOBTaskID Create( const char* name, JOBFunc&& func, const JOBSplit& split = JOBSplit::Single() );
    static JOBTaskID Create( const char* name, JOBFunc&& func, const JOBSplit& split, JOBFunc&& epilogue );
    static JOBTaskID Continuation( const JOBContext& ctx, const char* name, JOBFunc&& func, const JOBSplit& split = JOBSplit::Single() );
    
    static void AddChildren( JOBTaskID parent, JOBTaskID const* child, u32 nb_children );
    static void AddChildrenAndSpawn( JOBTaskID parent, JOBTaskID const* child, u32 nb_children, JOBPriority prio = JOBPriority::HIGH );
    static void Spawn( JOBTaskID task, JOBPriority prio = JOBPriority::HIGH );

    static void Wait( JOBTaskID task );
};

