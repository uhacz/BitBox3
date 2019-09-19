#include "input.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <XInput.h>

#include <math.h>
#include <float.h>
#include <stdio.h>

namespace bx
{

    namespace
    {
        inline void _GetAnalogStickValue( float* dst_value, SHORT src_value, float magnitude_inv, SHORT deadzone )
        {
            float value = 0.f;
            if( abs( src_value ) > deadzone )
            {
                value = (float)src_value * magnitude_inv;
            }

            *dst_value = value;
        }

        void UpdatePadInputState( BXInput::PadState* pad, const XINPUT_STATE& xstate )
        {

            const float analog_magnitude = 32767.f;
            const float analog_magnitude_inv = 1.f / analog_magnitude;
            const float analog_trigger_magnitude = 255.f;
            const float analog_trigger_magnitude_inv = 1.f / analog_trigger_magnitude;

            _GetAnalogStickValue( &pad->analog.left_X, xstate.Gamepad.sThumbLX, analog_magnitude_inv, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );
            _GetAnalogStickValue( &pad->analog.left_Y, xstate.Gamepad.sThumbLY, analog_magnitude_inv, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );

            _GetAnalogStickValue( &pad->analog.right_X, xstate.Gamepad.sThumbRX, analog_magnitude_inv, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );
            _GetAnalogStickValue( &pad->analog.right_Y, xstate.Gamepad.sThumbRY, analog_magnitude_inv, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

            _GetAnalogStickValue( &pad->analog.L2, xstate.Gamepad.bLeftTrigger, analog_trigger_magnitude_inv, XINPUT_GAMEPAD_TRIGGER_THRESHOLD );
            _GetAnalogStickValue( &pad->analog.R2, xstate.Gamepad.bRightTrigger, analog_trigger_magnitude_inv, XINPUT_GAMEPAD_TRIGGER_THRESHOLD );

            uint16_t src_buttons = xstate.Gamepad.wButtons;
            uint32_t buttons = 0;

            if( src_buttons & XINPUT_GAMEPAD_DPAD_UP )
                buttons |= BXInput::PadState::eUP;
            if( src_buttons & XINPUT_GAMEPAD_DPAD_DOWN )
                buttons |= BXInput::PadState::eDOWN;
            if( src_buttons & XINPUT_GAMEPAD_DPAD_LEFT )
                buttons |= BXInput::PadState::eLEFT;
            if( src_buttons & XINPUT_GAMEPAD_DPAD_RIGHT )
                buttons |= BXInput::PadState::eRIGHT;
            if( src_buttons & XINPUT_GAMEPAD_START )
                buttons |= BXInput::PadState::eSTART;
            if( src_buttons & XINPUT_GAMEPAD_BACK )
                buttons |= BXInput::PadState::eBACK;
            if( src_buttons & XINPUT_GAMEPAD_LEFT_THUMB )
                buttons |= BXInput::PadState::eLTHUMB;
            if( src_buttons & XINPUT_GAMEPAD_RIGHT_THUMB )
                buttons |= BXInput::PadState::eRTHUMB;
            if( src_buttons & XINPUT_GAMEPAD_LEFT_SHOULDER )
                buttons |= BXInput::PadState::eLSHOULDER;
            if( src_buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER )
                buttons |= BXInput::PadState::eRSHOULDER;
            if( src_buttons & XINPUT_GAMEPAD_A )
                buttons |= BXInput::PadState::eA;
            if( src_buttons & XINPUT_GAMEPAD_B )
                buttons |= BXInput::PadState::eB;
            if( src_buttons & XINPUT_GAMEPAD_X )
                buttons |= BXInput::PadState::eX;
            if( src_buttons & XINPUT_GAMEPAD_Y )
                buttons |= BXInput::PadState::eY;

            if( pad->analog.L2 > FLT_EPSILON )
                buttons |= BXInput::PadState::eLTRIGGER;

            if( pad->analog.R2 > FLT_EPSILON )
                buttons |= BXInput::PadState::eRTRIGGER;

            pad->digital.buttons = buttons;
        }
    }///
}//

void InputUpdatePad_XInput( BXInput::Pad* pads, uint32_t n )
{
    DWORD dwResult;
    for( DWORD i = 0; i < n; i++ )
    {
        XINPUT_STATE xstate;
        dwResult = XInputGetState( i, &xstate );

        BXInput::PadState* state = pads[i].CurrentState();

        state->connected = dwResult == ERROR_SUCCESS;
        if( !state->connected )
            continue;

        bx::UpdatePadInputState( state, xstate );


    #ifdef BX_DEBUG_PAD_STATE	
        const PadInputState& pad0 = state;
        printf( "%f ; %f | %f ; %f | %f ; %f \n", pad0.analog.left_X, pad0.analog.left_Y, pad0.analog.right_X, pad0.analog.right_Y, pad0.analog.L2, pad0.analog.R2 );
        printf( "%d ; %d | %d ; %d\n"
            , pad0.digital.buttons & PadInputState::eUP
            , pad0.digital.buttons & PadInputState::eDOWN
            , pad0.digital.buttons & PadInputState::eLEFT
            , pad0.digital.buttons & PadInputState::eRIGHT );

        printf( "A: %d ; B: %d ; X: %d ; Y: %d ; L1 : %d ; R1 : %d ; L2 : %d ; R2 : %d ; L3 : %d ; R3 : %d\n"
            , pad0.digital.buttons & PadInputState::eA
            , pad0.digital.buttons & PadInputState::eB
            , pad0.digital.buttons & PadInputState::eX
            , pad0.digital.buttons & PadInputState::eY
            , pad0.digital.buttons & PadInputState::eL1
            , pad0.digital.buttons & PadInputState::eR1
            , pad0.digital.buttons & PadInputState::eL2
            , pad0.digital.buttons & PadInputState::eR2
            , pad0.digital.buttons & PadInputState::eL3
            , pad0.digital.buttons & PadInputState::eR3
        );
    #endif
    }
}