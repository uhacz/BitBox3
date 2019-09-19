#include "pool.h"
#include <assert.h>

pool_t pool_t::create( void* memory, uint32_t memory_size_in_bytes, uint32_t chunk_size_in_bytes )
{
    assert( chunk_size_in_bytes >= sizeof( void* ) );
    assert( memory_size_in_bytes >= chunk_size_in_bytes );
    assert( (memory_size_in_bytes % chunk_size_in_bytes) == 0 );

    pool_t pool;
    pool._pool = (uintptr_t)memory;
    pool._free = (uintptr_t)memory;
    pool._memory_size = memory_size_in_bytes;
    pool._chunk_size = chunk_size_in_bytes;
    pool._allocated_size = 0;

    uint8_t* last_chunk = (uint8_t*)memory + memory_size_in_bytes - chunk_size_in_bytes;
    uint8_t* last_pointer = 0;

    while( last_chunk != memory )
    {
        *((uintptr_t*)last_chunk) = (uintptr_t)last_pointer;
        last_pointer = last_chunk;
        last_chunk -= chunk_size_in_bytes;
    }
    
    *((uintptr_t*)pool._free) = (uintptr_t)last_pointer;

    return pool;
}

void* pool_t::alloc()
{
    if( _free == 0 )
        return nullptr;

    void* result = (void*)_free;
    _free = *((uintptr_t*)_free);
    _allocated_size += _chunk_size;
    return result;
}

void pool_t::free( void* pointer )
{
    assert( (intptr_t)pointer >= (intptr_t)_pool );
    assert( (intptr_t)pointer < (intptr_t)((uint8_t*)_pool + _memory_size ) );
    assert( _allocated_size >= _chunk_size );

    _allocated_size -= _chunk_size;
    *((uintptr_t*)pointer) = _free;
    _free = (uintptr_t)pointer;
}

//
// dynamic_pool_t
//
#include <memory/memory.h>

inline uint32_t AlignValue32( uint32_t value, uint32_t alignment )
{
    assert( alignment && (alignment & (alignment - 1)) == 0 );
    alignment--;
    return ((value + alignment) & ~alignment);
}

inline uint64_t AlignValue64( uint64_t val, uint64_t alignment )
{
    assert( alignment && (alignment & (alignment - 1)) == 0 );
    alignment--;
    return ((val + alignment) & ~alignment);
}


static inline dynamic_pool_t::node_t* create_node( const dynamic_pool_t& dyn_pool )
{
    const uint32_t pool_mem_size = dyn_pool._num_chunks_per_pool * dyn_pool._chunk_size;
    uint32_t mem_size = 0;
    mem_size += sizeof( dynamic_pool_t::node_t );
    mem_size = AlignValue32( mem_size, dyn_pool._alignment );
    mem_size += pool_mem_size;

    dynamic_pool_t::node_t* node = (dynamic_pool_t::node_t*)BX_MALLOC( dyn_pool._backend_alloc, mem_size, dyn_pool._alignment );
    node->next = nullptr;

    void* pool_memory = (void*)AlignValue64( (uint64_t)( node + 1 ), dyn_pool._alignment);
    node->pool = pool_t::create( pool_memory, pool_mem_size, dyn_pool._chunk_size );

    return node;
}
static inline void destroy_node( dynamic_pool_t::node_t* node, BXIAllocator* allocator )
{
    BX_FREE( allocator, node );
}

dynamic_pool_t dynamic_pool_t::create( BXIAllocator* allocator, uint32_t chunk_size, uint32_t chunk_alignment, uint32_t num_chunks_per_pool )
{
    dynamic_pool_t dyn_pool;
    dyn_pool._backend_alloc = allocator;
    dyn_pool._chunk_size = chunk_size;
    dyn_pool._alignment = chunk_alignment;
    dyn_pool._num_chunks_per_pool = num_chunks_per_pool;
    dyn_pool._begin = create_node( dyn_pool );

    return dyn_pool;
}

void dynamic_pool_t::destroy( dynamic_pool_t* dyn_pool )
{
    node_t* node = dyn_pool->_begin;
    while( node )
    {
        node_t* to_delete = node;
        node = node->next;

        destroy_node( to_delete, dyn_pool->_backend_alloc );
    }
    dyn_pool[0] = {};
}

void* dynamic_pool_t::alloc()
{
    node_t* prev_node = _begin;
    node_t* node = _begin;
    while( node->pool.empty() )
    {
        prev_node = node;
        node = node->next;
        if( !node )
            break;
    }

    if( !node )
    {
        node = create_node( *this );
        prev_node->next = node;
    }

    return node->pool.alloc();
}

void dynamic_pool_t::free( void* pointer )
{
    node_t* node = _begin;

    while( node )
    {
        void* b = node->pool.begin();
        void* e = node->pool.end();

        if( b <= pointer && e > pointer )
        {
            break;
        }

        node = node->next;
    }

    if( !node )
    {
        assert( "Bad address" );
    }
    else
    {
        node->pool.free( pointer );
    }
}
