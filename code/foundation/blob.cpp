#include "blob.h"
#include <memory/memory.h>

blob_t blob_t::create( void* data, unsigned size )
{
    blob_t blob;
    blob.raw = data;
    blob.size = size;
    return blob;
}

blob_t blob_t::allocate( BXIAllocator* allocator, unsigned size, unsigned alignment )
{
    blob_t blob;
    blob.raw = BX_MALLOC( allocator, size, alignment );
    blob.size = size;
    blob.allocator = allocator;
    return blob;
}

void blob_t::destroy()
{
    if( allocator )
    {
        BX_FREE( allocator, raw );
        raw = nullptr;
        size = 0;
        allocator = nullptr;
    }
}
