#include "tag.h"
#include <string.h>

uint64_t to_Tag64( const char str[] )
{
    char tmp[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    const int str_len = (int)strlen( str );
    const int taglen = ( str_len > 8 ) ? 8 : str_len;

    memcpy( tmp, str, taglen );
    return tag64_t( tmp );
}
