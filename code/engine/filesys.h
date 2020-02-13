#pragma once

struct BXIAllocator;

struct FSYSRelative
{
    static constexpr unsigned PATH_LENGTH = 255;
    static constexpr unsigned PATH_SIZE = PATH_LENGTH + 1;
    
    char path[PATH_SIZE];
};
struct FSYSAbsolute
{
    static constexpr unsigned PATH_LENGTH = 511;
    static constexpr unsigned PATH_SIZE = PATH_LENGTH + 1;

    char path[PATH_SIZE];
};

struct FSYSPathView
{
    char* begin = nullptr;
    size_t count = 0;
};



struct FSYS
{
    

    
    // private
    static void StartUp( const char* root, BXIAllocator* allocator );
    static void ShutDown();
    struct Impl;
    Impl* impl;
};

extern FSYS* gFs();
