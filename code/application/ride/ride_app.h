#pragma once

#include "../application.h"
#include "../engine/low_level.h"


struct RideApplication : BXIApplication
{
    virtual bool Startup( int argc, const char** argv, BXWindow* win, BXIAllocator* allocator ) override;
    virtual void Shutdown( BXIAllocator* allocator ) override;
    virtual bool Update( BXWindow* win, unsigned long long deltaTimeUS, BXIAllocator* allocator ) override;

    ENGLowLevel _e;
};