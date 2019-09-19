#pragma once

#define LOGGER_ENABLED 1
#define ASSERTION_ENABLED 1

#ifdef __cplusplus
extern "C" {
#endif
//////////////////////////////////////////////////////////////////////////
extern void BXDebugAssert( int expression, const char *format, ... );
extern void BXDebugHalt( char *str );
extern void BXCheckFloat( float x ); 

extern void BXLog( const char* fmt, ... );

#ifdef __cplusplus
}
#endif

#ifdef LOGGER_ENABLED

#define SYS_LOG_INFO( ... ) BXLog( __VA_ARGS__ ); BXLog( " : at %s:%d - \n", __FILE__, __LINE__ )
#define SYS_LOG_WARNING( ... ) BXLog( "WARNING: " ); BXLog( __VA_ARGS__ ); BXLog( " : at %s:%d - \n", __FILE__, __LINE__ )
#define SYS_LOG_ERROR( ... ) BXLog( "ERROR: " ); BXLog( __VA_ARGS__ ); BXLog( " : at %s:%d - \n", __FILE__, __LINE__ )

#else

#define SYS_LOG_INFO( ... )
#define SYS_LOG_WARNING( ... )
#define SYS_LOG_ERROR( ... )

#endif

#ifdef ASSERTION_ENABLED

#define AT __FILE__ ":" MAKE_STR(__LINE__)
#define SYS_ASSERT( expression ) BXDebugAssert( expression, "ASSERTION FAILED " AT " " #expression "\n" )
#define SYS_ASSERT_TXT( expression, txt, ... ) BXDebugAssert( expression, "ASSERTION FAILED " AT " " #expression "\n" #txt, __VA_ARGS__ )

#define SYS_STATIC_ASSERT( expression ) static_assert( expression, "" )
#define SYS_NOT_IMPLEMENTED SYS_ASSERT( false && "not implemented" )

#else
#define SYS_ASSERT( expression )
#define SYS_ASSERT_TXT( expression, txt )
#define SYS_STATIC_ASSERT( expression )
#endif



