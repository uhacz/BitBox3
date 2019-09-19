#include "rw_spin_lock.h"
#include "../type.h"
#include "../debug.h"

//http://joeduffyblog.com/2009/01/29/a-singleword-readerwriter-spin-lock/
//https://github.com/boytm/rw_spinlock/blob/master/spinlock.cpp

static constexpr int UNLOCKED_VALUE = 0;
static constexpr int LOCK_WRITE_VALUE = -1;

rw_spin_lock_t::rw_spin_lock_t()
    :_value( 0 )
{}

void rw_spin_lock_t::lock_read()
{
    int expected;
    int desired;

    while( true )
    {
        expected = std::atomic_load_explicit( &_value, std::memory_order_relaxed );

        if( expected >= 0 )
        {
            desired = 1 + expected;
            if( std::atomic_compare_exchange_weak_explicit(
                &_value, &expected, desired,
                std::memory_order_relaxed, std::memory_order_relaxed ) )
            {
                break; // success
            }
        }
    }

    std::atomic_thread_fence( std::memory_order_acquire ); // sync
}

void rw_spin_lock_t::unlock_read()
{
    int expected;
    int desired;

    while( true )
    {
        expected = std::atomic_load_explicit( &_value, std::memory_order_relaxed );
        SYS_ASSERT( expected > 0 );

        desired = expected - 1;

        std::atomic_thread_fence( std::memory_order_release ); // sync
        if( std::atomic_compare_exchange_weak_explicit( &_value, &expected, desired, std::memory_order_relaxed, std::memory_order_relaxed ) )
            break; // success
    }
}

void rw_spin_lock_t::lock_write()
{
    int expected;
    int desired;

    while( true )
    {
        expected = std::atomic_load_explicit( &_value, std::memory_order_relaxed );

        if( expected == UNLOCKED_VALUE )
        {
            desired = LOCK_WRITE_VALUE;
            if( std::atomic_compare_exchange_weak_explicit( &_value, &expected, desired, std::memory_order_relaxed, std::memory_order_relaxed ) )
                break; // success
        }
    }

    std::atomic_thread_fence( std::memory_order_release ); // sync
}

void rw_spin_lock_t::unlock_write()
{
    int expected;
    int desired;

    while( true )
    {
        expected = std::atomic_load_explicit( &_value, std::memory_order_relaxed );
        SYS_ASSERT( expected == LOCK_WRITE_VALUE );

        desired = UNLOCKED_VALUE;

        std::atomic_thread_fence( std::memory_order_release ); // sync
        if( std::atomic_compare_exchange_weak_explicit( &_value, &expected, desired, std::memory_order_relaxed, std::memory_order_relaxed ) )
            break; // success
    }
}
