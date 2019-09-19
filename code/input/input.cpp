#include "input.h"
#include <string.h>

void InputClear( BXInput* input, bool keys, bool mouse, bool pad )
{
    if( keys )
    {
        memset( input->kbd.CurrentState(), 0, sizeof( BXInput::KeyboardState ) );
    }
    if( mouse )
    {
        memset( input->mouse.CurrentState(), 0, sizeof( BXInput::MouseState ) );
    }
    if( pad )
    {
        memset( input->pad.CurrentState(), 0, sizeof( BXInput::PadState ) );
    }
}

void InputSwap( BXInput* input )
{
    BXInput::Swap( &input->kbd );
    BXInput::Swap( &input->mouse );
    BXInput::Swap( &input->pad );
}

void BXInput::ComputeMouseDelta( Mouse* mouse )
{
    auto curr = mouse->CurrentState();
    auto prev = mouse->PreviousState();
    curr->dx = curr->x - prev.x;
    curr->dy = curr->y - prev.y;
}
