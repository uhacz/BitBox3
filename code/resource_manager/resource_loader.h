#pragma once

#include <foundation/type.h>
#include <memory/memory.h>

struct BXIAllocator;

struct RSMResourceData
{
    const void* pointer = nullptr;
    size_t size = 0;
    BXIAllocator* allocator = nullptr;
};

struct RSMLoader
{
    virtual ~RSMLoader() {}

    virtual const char* SupportedType() const = 0;
    virtual bool IsBinary() const = 0;
    virtual bool Load( RSMResourceData* out, const void* data, uint32_t size, BXIAllocator* allocator, void* system );
    virtual void Unload( RSMResourceData* in_out );
};

using RSMLoaderCreator = RSMLoader*(BXIAllocator* allocator);
#define RSM_DEFINE_LOADER( cls )\
    static inline RSMLoader* Internal_Creator( BXIAllocator* allocator )\
    {\
        return BX_NEW( allocator, cls );\
    }