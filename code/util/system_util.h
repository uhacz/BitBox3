#pragma once

template<typename TSys, typename TImpl, typename ...ImplArgs >
TSys* System_StartUp( BXIAllocator* allocator, ImplArgs&&... args )
{
    const u32 memory_size = sizeof( TSys ) + sizeof( TImpl );
    void* memory = BX_MALLOC( allocator, memory_size, sizeof( void* ) );
    TSys* sys = new(memory) TSys();
    TImpl* impl = new(sys + 1) TImpl();

    sys->impl = impl;
    impl->StartUp( allocator, std::forward<ImplArgs>( args )... );

    return sys;
}

template<typename TSys, typename TImpl, typename ...ImplArgs >
void System_ShutDown( TSys** sys, ImplArgs&& ...args )
{
    if( !sys[0] )
        return;

    TImpl* impl = sys[0]->impl;
    BXIAllocator* allocator = impl->_allocator;

    impl->ShutDown( std::forward<ImplArgs>( args )... );
    InvokeDestructor( impl );
    InvokeDestructor( sys );

    BX_FREE0( allocator, sys[0] );
}
