#pragma once

#include <foundation/type.h>
#include <input/input.h>

struct BXWindow
{
    enum { NAME_LEN = 63 };
    char name[NAME_LEN+1] = {};
    uint32_t width        = 0;
    uint32_t height       = 0;
    uint32_t full_screen  = 0;
    BXInput input         = {};

    uintptr_t( *GetSystemHandle )( const BXWindow* instance );
};
