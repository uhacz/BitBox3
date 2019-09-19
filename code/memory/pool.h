#pragma once

#include <stdint.h>

struct pool_t
{
    static pool_t create( void* memory, uint32_t memory_size_in_bytes, uint32_t chunk_size_in_bytes );

    void* alloc();
    void free( void* pointer );
    bool empty() const { return _free == 0; }
    void* begin() const { return (void*)_pool; }
    void* end() const { return (void*)((uint8_t*)_pool + _memory_size); }

    // data
    uintptr_t _free = 0;
    uintptr_t _pool = 0;
    uint32_t _memory_size = 0;
    uint32_t _chunk_size = 0;
    uint32_t _allocated_size = 0;
};


struct BXIAllocator;
struct dynamic_pool_t
{
    static dynamic_pool_t create( BXIAllocator* allocator, uint32_t chunk_size_in_bytes, uint32_t chunk_alignment, uint32_t num_chunks_per_pool );
    static void destroy( dynamic_pool_t* dyn_pool );

    void* alloc();
    void free( void* pointer );
    
    // data
    struct node_t
    {
        node_t* next = nullptr;
        pool_t pool;
    };

    node_t* _begin = nullptr;
    BXIAllocator* _backend_alloc = nullptr;
    uint32_t _alignment = sizeof( void* );
    uint32_t _num_chunks_per_pool = 0;
    uint32_t _chunk_size = 0;
};