#pragma once

#include <stdint.h>
#include <atomic>

class semaphore_t
{
public:
	semaphore_t( int32_t initialCount = 0 );
	~semaphore_t();
	
	void signal( int32_t count = 1 );
	void wait();

private:
	semaphore_t(const semaphore_t& other) = delete;
	semaphore_t& operator=(const semaphore_t& other) = delete;

private:
	uintptr_t _handle = 0;

};

//--- implementation taken from: https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h
class light_semaphore_t
{

public:
	light_semaphore_t( int initialCount = 0 );

	bool try_wait();
	void wait();
	void signal( int count = 1 );

private:
	void wait_with_partial_spinning();

private:
	std::atomic<int> _count;
	semaphore_t _sema;
};

