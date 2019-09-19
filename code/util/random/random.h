#pragma once

#include "pcg_basic.h"

struct random_t
{
    pcg32_random_t _impl;
};

random_t RandomInit( uint64_t seed, uint64_t stream );

uint32_t Random( random_t* rnd );

// from 0 to max-1
uint32_t Random( random_t* rnd, uint32_t max );

// from 0 to 1
float Randomf( random_t* rnd );

float Randomf( random_t* rnd, float min, float max );

