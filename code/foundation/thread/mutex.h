#pragma once

struct mutex_t
{
    mutex_t( unsigned spin_count = 64 );
    ~mutex_t();
    void lock();
    void unlock();

private:
    static constexpr unsigned INTERNAL_DATA_SIZE = 128;
    char _prv[INTERNAL_DATA_SIZE];
};


template< typename T >
struct scope_lock_t
{
    scope_lock_t( T& lock )
        :_lock(lock)
    {
        _lock.lock();
    }
    ~scope_lock_t()
    {
        _lock.unlock();
    }

    T& _lock;
};

using scope_mutex_t = scope_lock_t<mutex_t>;