#include "resource_loader.h"
#include <memory\memory.h>
#include <string.h>

bool RSMLoader::Load( RSMResourceData* out, const void* data, uint32_t size, BXIAllocator* allocator, void* system )
{
    out->allocator = allocator;
    out->pointer = data;
    out->size = size;

    return out->size != 0;
}

void RSMLoader::Unload( RSMResourceData* in_out )
{
    // do nothing. Memory will be deallocated by manager
}
