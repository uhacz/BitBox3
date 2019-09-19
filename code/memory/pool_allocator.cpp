#include "pool_allocator.h"

template< typename TAlloc >
static void* PoolAlloc( BXIAllocator* _this, size_t size, size_t align )
{
    TAlloc* allocator = (TAlloc*)_this;
    void* ptr = allocator->_pool.alloc();
#if _DEBUG
    if( !ptr )
    {
        // OOM
        __debugbreak();
    }
#endif
    return ptr;
}

template< typename TAlloc >
static void PoolFree( BXIAllocator* _this, void* ptr )
{
    TAlloc* allocator = (TAlloc*)_this;
    allocator->_pool.free( ptr );
}

#if MEM_USE_DEBUG_ALLOC == 1
template< typename TAlloc >
static void* DebugPoolAlloc( BXIAllocator* _this, size_t size, size_t align, const char* file, size_t line, const char* func )
{
    return PoolAlloc<TAlloc>( _this, size, align );
}

template< typename TAlloc >
static void DebugPoolFree( BXIAllocator* _this, void* ptr )
{
    return PoolFree<TAlloc>( _this, ptr );
}
#endif

void PoolAllocator::Create( PoolAllocator* allocator, void* memory, size_t size, size_t chunk_size )
{
    allocator->_pool = pool_t::create( memory, (uint32_t)size, (uint32_t)chunk_size );
    allocator->Alloc = PoolAlloc<PoolAllocator>;
    allocator->Free = PoolFree<PoolAllocator>;

#if MEM_USE_DEBUG_ALLOC == 1
    allocator->DbgAlloc = DebugPoolAlloc<PoolAllocator>;
    allocator->DbgFree = DebugPoolFree<PoolAllocator>;
#endif
}

void PoolAllocator::Destroy( PoolAllocator* allocator )
{
    (void)allocator;
}

//
//
//
void DynamicPoolAllocator::Create( DynamicPoolAllocator* allocator, BXIAllocator* backent_allocator, size_t chunk_size, size_t alignment, size_t num_chunks_per_pool )
{
    allocator->_pool = dynamic_pool_t::create( backent_allocator, (uint32_t)chunk_size, (uint32_t)alignment, (uint32_t)num_chunks_per_pool );
    allocator->Alloc = PoolAlloc<DynamicPoolAllocator>;
    allocator->Free = PoolFree<DynamicPoolAllocator>;

#if MEM_USE_DEBUG_ALLOC == 1
    allocator->DbgAlloc = DebugPoolAlloc<DynamicPoolAllocator>;
    allocator->DbgFree = DebugPoolFree<DynamicPoolAllocator>;
#endif
}

void DynamicPoolAllocator::Destroy( DynamicPoolAllocator* allocator )
{
    dynamic_pool_t::destroy( &allocator->_pool );
}
