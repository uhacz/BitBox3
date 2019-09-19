#pragma once

#include "../application.h"

struct RideApplication : BXIApplication
{
    virtual bool Startup( int argc, const char** argv, BXIAllocator* allocator ) override;
    virtual void Shutdown( BXIAllocator* allocator ) override;
    virtual bool Update( BXWindow* win, unsigned long long deltaTimeUS, BXIAllocator* allocator ) override;
};