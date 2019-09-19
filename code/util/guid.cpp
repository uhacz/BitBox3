#include "guid.h"

#include <combaseapi.h>
#include "foundation/type.h"
#include "foundation/debug.h"

#pragma comment(lib, "Shlwapi.lib")

__declspec(dllimport)
BOOL WINAPI GUIDFromStringA( LPCSTR psz, LPGUID pguid );

union guid_converter_t
{
    guid_t my;
    GUID os;
};

guid_t GenerateGUID()
{
    guid_converter_t tmp;
    CoCreateGuid( &tmp.os );
    return tmp.my;
}

void ToString( guid_string_t* str, const guid_t& g )
{
    guid_converter_t tmp;
    tmp.my = g;

    wchar_t uni[guid_string_t::SIZE] = {};
    StringFromGUID2( tmp.os, uni, guid_string_t::SIZE );

    size_t dummy;
    wcstombs_s( &dummy, str->data, uni, guid_string_t::SIZE );
    
}

void FromString( guid_t* g, const guid_string_t& str )
{
    wchar_t uni[guid_string_t::SIZE] = {};
    size_t uni_len = 0;
    mbstowcs_s( &uni_len, uni, str.data, strlen( str.data ) );

    guid_converter_t tmp;
    CLSIDFromString( uni, &tmp.os );
    g[0] = tmp.my;
}

void FromString( guid_t* g, const char* str )
{
    const size_t str_len = strlen( str );
    SYS_ASSERT( str_len < guid_string_t::SIZE );
    
    wchar_t uni[guid_string_t::SIZE] = {};
    size_t uni_len = 0;
    mbstowcs_s( &uni_len, uni, str, str_len );

    guid_converter_t tmp;
    CLSIDFromString( uni, &tmp.os );
    g[0] = tmp.my;
}
