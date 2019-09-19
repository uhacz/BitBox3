#pragma once

template<typename Type>
Type LowPassFilter( const Type& raw, const Type& current, float rc, float dt )
{
    const float a = dt / ( rc + dt );
    return current * ( 1.f - a ) + raw * a;
}