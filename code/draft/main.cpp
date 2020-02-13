#include <stdlib.h>
#include "xmmintrin.h"

#include "../foundation/math/vmath.h"
#include "../util/random/random.h"
#include "foundation/time.h"
#include "stdio.h"

#include "../job/job.h"
#include "assert.h"

static u32 _tier0 = 0;
static u32 _tier1 = 0;
static u32 _tier2 = 0;

int main()
{
    JOB::StartUp();
    
    int* arr = new int[1024];
    
    JOBTaskID job0 = JOB::Create( "test0", [arr]( const JOBRange range, const JOBContext& ctx )
    {
        printf( "Begin test.\n" );
        _tier0 = 1;

        JOBTaskID job = JOB::Create( "test", [arr]( const JOBRange range, const JOBContext& ctx )
        {
            assert( _tier0 != 0 );
            random_t rnd = RandomInit( 0xBAADC0DEDEADF00D, ctx.thread_index );

            for( u32 i = range.begin; i < range.end; ++i )
            {
                const u32 value = Random( &rnd, 1024 );
                printf( "thread: %u -> data index: %u -> value: %u!\n", ctx.thread_index, i, value );
                fflush( stdout );
            }
        }, JOBSplit::Chunk( 1024, 64 ), 
        []( const JOBRange range, const JOBContext& ctx ) 
        { 
            assert( _tier0 != 0 ); 
            _tier1 = 1; 
        } );

        JOBTaskID epilogue = JOB::Continuation( ctx, "test1", []( const JOBRange range, const JOBContext& ctx )
        {
            assert( _tier1 != 0 );
            _tier2 = 1;
            fflush( stdout );
            printf( "End test!\n" );
        } );

        JOB::AddChildrenAndSpawn( epilogue, &job, 1 );
    } );
    JOB::Spawn( job0 );
    JOB::Wait( job0 );

    assert( _tier2 != 0 );

    JOB::ShutDown();

    delete[] arr;

    system( "PAUSE" );
    return 0;
}