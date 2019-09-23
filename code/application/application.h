#pragma once

struct BXIAllocator;
struct BXWindow;

struct BXIApplication
{
    virtual bool Startup( int argc, const char** argv, BXWindow* win, BXIAllocator* allocator ) = 0;
    virtual void Shutdown( BXIAllocator* allocator ) = 0;
    virtual bool Update( BXWindow* win, unsigned long long deltaTimeUS, BXIAllocator* allocator ) = 0;
};

BXIApplication* CreateApplication( const char* name, BXIAllocator* allocator );
void DestroyApplication( BXIApplication** app, BXIAllocator* allocator );