#include "file_system_name.h"

#include <foundation/type.h>
#include <foundation/debug.h>
#include <string.h>

void FSName::Clear()
{
    _total_length = 0;
    _relative_path_offset = 0;
    _relative_path_length = 0;
    _data[0] = 0;
}

bool FSName::Append( const char * str )
{
    const int32_t len = (int32_t)strlen( str );
    const int32_t chars_left = MAX_LENGTH - _total_length;

    if( chars_left < len )
        return false;

    memcpy( _data + _total_length, str, len );
    _total_length += len;
    SYS_ASSERT( _total_length <= MAX_LENGTH );
    _data[_total_length] = 0;

    return true;
}

bool FSName::AppendRelativePath( const char * str )
{
    const uint32_t offset = _total_length;
    if( Append( str ) )
    {
        _relative_path_offset = offset;
        _relative_path_length = _total_length - offset;
        return true;
    }
    return false;
}