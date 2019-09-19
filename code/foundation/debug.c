#include "debug.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <crtdbg.h>
#include <stdarg.h>
#include <stdio.h>
#include <float.h>

#ifdef _MBCS
#pragma warning( disable: 4996 )
#endif

void BXDebugAssert( int expression, const char *format, ... )
{
    char str[1024];

	if( !expression )
	{
		va_list arglist;
		va_start( arglist, format );
		vsprintf_s( str, 1024, format, arglist );
		va_end( arglist );
		BXDebugHalt( str );
	}
}

void BXDebugHalt( char *str )
{
	MessageBox( 0, str, "error", MB_OK );
	//__asm { int 3 }
	__debugbreak();
}   

void BXCheckFloat( float x )
{
#ifdef _MSC_VER
    const double d = (double)x;
    const int cls = _fpclass( d );
    if( cls == _FPCLASS_SNAN ||
        cls == _FPCLASS_QNAN ||
        cls == _FPCLASS_NINF ||
        cls == _FPCLASS_PINF )
    {
        BXDebugAssert( 0, "invalid float" );
    }
#else
    (void)x;
#endif
}

extern void BXLog( const char* fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    vprintf_s( fmt, args );
    va_end( args );
}
