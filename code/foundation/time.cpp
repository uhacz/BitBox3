#include "time.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

static inline int64_t herz()
{
    LARGE_INTEGER tf;
    QueryPerformanceFrequency( &tf );
    return tf.QuadPart;
}

uint64_t BXTime::GlobalTimeMS()
{
    static uint64_t clockPerSec = herz();
    LARGE_INTEGER ct;
    QueryPerformanceCounter( &ct );

    return unsigned( ct.QuadPart / ( clockPerSec / 1000 ) );
}

uint64_t BXTime::GlobalTimeUS()
{
    static uint64_t clockPerSec = herz();
    static double clockPerUSrcp = 1000000.0 / clockPerSec;
    LARGE_INTEGER ct;
    QueryPerformanceCounter( &ct );

    return (uint64_t)( ct.QuadPart * clockPerUSrcp );
}

void BXTime::Sleep( uint64_t ms )
{
    ::Sleep( (DWORD)ms );
}

BXTimeQuery BXTimeQuery::Begin()
{
    BXTimeQuery tq;
    LARGE_INTEGER ct;
    QueryPerformanceCounter( &ct );
    tq.tick_start = ct.QuadPart;
    return tq;
}

void BXTimeQuery::End( BXTimeQuery* tq )
{
    LARGE_INTEGER ct;
    BOOL bres = QueryPerformanceCounter( &ct );
    tq->tick_stop = ct.QuadPart;

    LARGE_INTEGER tf;
    bres = QueryPerformanceFrequency( &tf );

    const uint64_t numTicks = ( (uint64_t)( tq->tick_stop - tq->tick_start ) );
    const double durationUS = double( numTicks * 1000000 ) / (double)tf.QuadPart;
    tq->duration_US = (uint64_t)durationUS;
}
