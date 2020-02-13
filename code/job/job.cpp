#include "job.h"

#include "../3rd_party/gts/include/gts/micro_scheduler/MicroScheduler.h"
#include "../3rd_party/gts/include/gts/micro_scheduler/WorkerPool.h"

#include <utility>


static inline JOBTaskID ToTaskID( gts::Task* task )
{
    return { (u64)task };
}
static inline gts::Task* ToTask( JOBTaskID counter )
{
    return (gts::Task*)counter.impl;
}

namespace job
{
    static gts::WorkerPool g_worker_pool = {};
    static gts::MicroScheduler g_scheduler = {};

    struct EmptyTask
    {
        static gts::Task* taskFunc( gts::Task* this_task, gts::TaskContext const& ctx )
        {
            return nullptr;
        }
    };

    struct Task
    {
        Task( const JOBFunc& func, const JOBRange& range, JOBFunc&& epilogue = nullptr )
            : _func( func ), _range( range ), _epilogue( std::move( epilogue ) )
        {}

        Task( JOBFunc&& func, const JOBRange& range, JOBFunc&& epilogue = nullptr )
            : _func( std::move( func ) ), _range( range ), _epilogue( std::move( epilogue ) )
        {}

        JOBFunc _func;
        JOBFunc _epilogue;
        JOBRange _range;

        static gts::Task* taskFunc( gts::Task* this_task, gts::TaskContext const& ctx )
        {
            JOBContext my_ctx;
            my_ctx.thread_index = ctx.workerIndex;
            my_ctx.this_job = ToTaskID( this_task );

            Task* task_data = (Task*)this_task->getData();
            task_data->_func( task_data->_range, my_ctx );

            if( task_data->_epilogue )
            {
                JOBRange range;
                range.begin = 0;
                range.end = 1;
                gts::Task* epilog = ctx.pMicroScheduler->allocateTask<job::Task>( task_data->_epilogue, range );
                return epilog;
            }

            return nullptr;
        }
    };

    struct ParallelForTask
    {
        ParallelForTask( JOBFunc&& func, const JOBSplit& split, JOBFunc&& epilogue = nullptr )
            : _func( std::move( func ) ), _split( split ), _epilogue( std::move( epilogue ) )
        {}
        
        JOBFunc _func;
        JOBFunc _epilogue;
        JOBSplit _split;

        static gts::Task* taskFunc( gts::Task* this_task, gts::TaskContext const& ctx )
        {
            ParallelForTask* task_data = (ParallelForTask*)this_task->getData();

            JOBContext my_ctx;
            my_ctx.thread_index = ctx.workerIndex;
            my_ctx.this_job = ToTaskID( this_task );

            const JOBSplit split = task_data->_split;
            const JOBFunc& func = task_data->_func;

            gts::Task* root_task = this_task;

            if( task_data->_epilogue )
            {
                JOBRange range;
                range.begin = 0;
                range.end = 1;
                gts::Task* epilog = ctx.pMicroScheduler->allocateTask<job::Task>( task_data->_epilogue, range );
                this_task->setContinuationTask( epilog );
                root_task = epilog;
            }

            const u32 nb_tasks = iceil( split.size, split.chunk );
            root_task->addRef( nb_tasks );


            for( u32 i = 0; i < nb_tasks; ++i )
            {
                JOBRange range;
                range.begin = i * split.chunk;
                range.end = min_of_2( range.begin + split.chunk, split.size );

                gts::Task* child = ctx.pMicroScheduler->allocateTask<job::Task>( func, range );
                root_task->addChildTaskWithoutRef( child );

                if( i < (nb_tasks - 1) )
                    ctx.pMicroScheduler->spawnTask( child );
                else
                    return child;
            }
            return nullptr;
        }
    };

    struct WaitTask
    {
        WaitTask( gts::Task* epilogue )
            : _epilogue( epilogue )
        {}

        gts::Task* _epilogue;
        static gts::Task* taskFunc( gts::Task* this_task, gts::TaskContext const& ctx )
        {
            this_task->waitForChildren( ctx );
            WaitTask* task_data = (WaitTask*)this_task->getData();
            return task_data->_epilogue;
        }
    };
}//

#pragma comment( lib, "../3rd_party/gts/lib/Debug/gts.lib" )

void JOB::StartUp( u32 nbThreads )
{
    job::g_worker_pool.initialize( nbThreads );
    job::g_scheduler.initialize( &job::g_worker_pool, (u32)JOBPriority::LOW + 1 );
}

void JOB::ShutDown()
{
    job::g_scheduler.shutdown();
    job::g_worker_pool.shutdown();
}

u32 JOB::GetThreadCount()
{
    return job::g_scheduler.workerCount();
}

static inline bool operator == ( const JOBSplit& a, const JOBSplit& b )
{
    return a.size == b.size && a.chunk == b.chunk;
}

static gts::Task* CreateTask( const char* name, JOBFunc&& func, const JOBSplit split, JOBFunc&& epilogue = nullptr )
{
    gts::Task* task = nullptr;
    if( split == JOBSplit::Single() )
    {
        JOBRange range;
        range.begin = 0;
        range.end = 1;
        task = job::g_scheduler.allocateTask<job::Task>( std::move( func ), range, std::move( epilogue ) );
    }
    else
    {
        task = job::g_scheduler.allocateTask<job::ParallelForTask>( std::move( func ), split, std::move( epilogue ) );
    }

    return task;
}

JOBTaskID JOB::Continuation( const JOBContext& ctx, const char* name, JOBFunc&& func, const JOBSplit& split )
{
    gts::Task* parent_task = ToTask( ctx.this_job );
    gts::Task* task = CreateTask( name, std::move( func ), split );
    parent_task->setContinuationTask( task );
    return ToTaskID( task );
}

JOBTaskID JOB::Create( const char* name, JOBFunc&& func, const JOBSplit& split )
{
    gts::Task* task = CreateTask( name, std::move( func ), split );
    return ToTaskID( task );
}

JOBTaskID JOB::Create( const char* name, JOBFunc&& func, const JOBSplit& split, JOBFunc&& epilogue )
{
    gts::Task* task = CreateTask( name, std::move( func ), split, std::move( epilogue ) );
    return ToTaskID( task );
}

void JOB::AddChildren( JOBTaskID parent, JOBTaskID const* child, u32 nb_children )
{
    gts::Task* parent_task = ToTask( parent );
    parent_task->addRef( nb_children );
    for( u32 i = 0; i < nb_children; ++i )
    {
        gts::Task* child_task = ToTask( child[i] );
        parent_task->addChildTaskWithoutRef( child_task );
    }
}

void JOB::AddChildrenAndSpawn( JOBTaskID parent, JOBTaskID const* child, u32 nb_children, JOBPriority prio /*= JOBPriority::HIGH */ )
{
    AddChildren( parent, child, nb_children );
    for( u32 i = 0; i < nb_children; ++i )
    {
        gts::Task* child_task = ToTask( child[i] );
        job::g_scheduler.spawnTask( child_task, (u32)prio );
    }
}

void JOB::Spawn( JOBTaskID task, JOBPriority prio )
{
    job::g_scheduler.spawnTask( ToTask( task ), (u32)prio );
}

gts::Task* CreateWaitTask( gts::Task* wait_for, gts::Task* run_after )
{
    gts::Task* wait_task = job::g_scheduler.allocateTask<job::WaitTask>( run_after );
    u32 nb_children = 0;
    nb_children += (wait_for) ? 1 : 0;
    nb_children += (run_after) ? 1 : 0;
    wait_task->addRef( nb_children );
    if( wait_for )
        wait_task->addChildTaskWithoutRef( wait_for );
    if( run_after )
        wait_task->addChildTaskWithoutRef( run_after );

    return wait_task;
}

void JOB::Wait( JOBTaskID task_id )
{
    gts::Task* task = ToTask( task_id );
    gts::Task* wait_task = CreateWaitTask( task, nullptr );
    job::g_scheduler.spawnTaskAndWait( wait_task, (u32)JOBPriority::HIGH );
}

#ifdef USE_ENKITS
#include "../3rd_party/enkiTS/TaskScheduler.h"

namespace job
{
    static enki::TaskScheduler g_scheduler;
}//

void JOB::StartUp( u32 nbThreads )
{
    enki::TaskSchedulerConfig cfg;
    cfg.numTaskThreadsToCreate = (nbThreads ) ? nbThreads : enki::GetNumHardwareThreads() - 1;

    job::g_scheduler.Initialize( cfg );
}

void JOB::ShutDown()
{
    job::g_scheduler.WaitforAllAndShutdown();
}
#endif