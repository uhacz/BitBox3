#include "serializer.h"

SRLInstance SRLInstance::CreateReader( uint32_t v, const void* data, uint32_t data_size, BXIAllocator* allocator )
{
    SRLInstance srl;
    srl.version = v;
    data_buffer::create( &srl.data, (void*)data, data_size );
    data_buffer::seek_write( &srl.data, data_size );
    srl.allocator = allocator;
    srl.is_writting = 0;

    return srl;
}

SRLInstance SRLInstance::CreateWriterStatic( uint32_t v, const void* data, uint32_t data_size, BXIAllocator* allocator )
{
    SRLInstance srl;
    srl.version = v;
    data_buffer::create( &srl.data, (void*)data, data_size );
    srl.allocator = allocator;
    srl.is_writting = 1;

    return srl;
}

SRLInstance SRLInstance::CreateWriterDynamic( uint32_t v, uint32_t data_size, BXIAllocator* allocator )
{
    SRLInstance srl;
    srl.version = v;
    data_buffer::create( &srl.data, data_size, allocator );
    srl.allocator = allocator;
    srl.is_writting = 1;

    return srl;
}
