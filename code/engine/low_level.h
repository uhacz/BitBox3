#pragma once

struct BXIAllocator;
struct RDIDevice;
struct RDICommandQueue;
struct BXWindow;

struct ENGLowLevel
{
    BXIAllocator* allocator = nullptr;
    RDIDevice* rdidev = nullptr;
    RDICommandQueue* rdicmdq = nullptr;

    static bool Startup( ENGLowLevel* e, int argc, const char** argv, BXWindow* window, BXIAllocator* main_allocator );
    static void Shutdown( ENGLowLevel* e );
};
