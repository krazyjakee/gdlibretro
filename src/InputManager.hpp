#pragma once

#include "godot_cpp/classes/input_event.hpp"
#include "libretro.h"

class InputManager
{
public:
    void poll();
    int16_t state( unsigned port, unsigned device, unsigned index, unsigned id );
    void forward_event( const godot::Ref<godot::InputEvent> &event,
                        retro_keyboard_event_t keyboard_callback );

private:
    bool keyboard_state[RETROK_LAST] = {};
    bool joypad_state[16] = {};
};
