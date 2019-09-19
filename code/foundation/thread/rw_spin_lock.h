#pragma once

#include <atomic>

struct rw_spin_lock_t
{
public:
    rw_spin_lock_t();

    void lock_read();
    void unlock_read();

    void lock_write();
    void unlock_write();

private:
    std::atomic<int> _value;
};

struct scoped_read_spin_lock_t
{
    scoped_read_spin_lock_t( rw_spin_lock_t& lock )
        :_lock( lock ) 
    {
        _lock.lock_read();
    }
    ~scoped_read_spin_lock_t()
    {
        _lock.unlock_read();
    }

    rw_spin_lock_t& _lock;
};

struct scoped_write_spin_lock_t
{
    scoped_write_spin_lock_t( rw_spin_lock_t& lock )
        :_lock( lock )
    {
        _lock.lock_write();
    }
    ~scoped_write_spin_lock_t()
    {
        _lock.unlock_write();
    }

    rw_spin_lock_t& _lock;
};