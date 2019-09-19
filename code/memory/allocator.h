#pragma once

#define MEM_USE_DEBUG_ALLOC 1


struct BXIAllocator
{
    void* ( *Alloc )( BXIAllocator* _this, size_t size, size_t align );
    void  ( *Free )( BXIAllocator* _this, void* ptr );

#if MEM_USE_DEBUG_ALLOC == 1
    void* ( *DbgAlloc )(BXIAllocator* _this, size_t size, size_t align, const char* file, size_t line, const char* func);
    void  ( *DbgFree )(BXIAllocator* _this, void* ptr );
#endif
};


