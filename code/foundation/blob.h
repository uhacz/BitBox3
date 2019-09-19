#pragma once

struct BXIAllocator;

struct blob_t
{
    union
    {
        void* raw = nullptr;
        char* txt;
        unsigned char* uchar;
        unsigned short* ushort;
        unsigned int* uint;
        float* flt;
    };

    size_t size = 0;
    BXIAllocator* allocator = nullptr;

    static blob_t create( void* data, unsigned size );
    static blob_t allocate( BXIAllocator* allocator, unsigned size, unsigned alignment );
    
    bool empty() const { return raw == nullptr; }
    void destroy();
};
