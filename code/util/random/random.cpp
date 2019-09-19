#include "random.h"

random_t RandomInit( uint64_t seed, uint64_t stream )
{
    random_t rnd;
    rnd._impl.state = seed;
    rnd._impl.inc = stream | 0x1;
    return rnd;
}

uint32_t Random( random_t* rnd )
{
    return pcg32_random_r( &rnd->_impl );
}

uint32_t Random( random_t* rnd, uint32_t max )
{
    return pcg32_boundedrand_r( &rnd->_impl, max );
}

float Randomf( random_t* rnd )
{
    static const float MAX_RAND_INV = 1.f / (float)( (uint32_t)(1 << 31) - 1 );
    return Random( rnd ) * MAX_RAND_INV;
}

float Randomf( random_t* rnd, float min, float max )
{
    return ( Randomf( rnd ) + min ) * (max - min);
}
