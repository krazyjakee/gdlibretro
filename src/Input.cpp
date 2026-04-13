#include "RetroHost.hpp"
#include "KeyboardMap.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

static bool keyboard_state[RETROK_LAST] = { false };
static bool joypad_state[16] = { false };

// Default keyboard-to-joypad mapping
static int godotKeyToJoypadButton( godot::Key key )
{
    switch ( key )
    {
        case godot::Key::KEY_UP:
            return RETRO_DEVICE_ID_JOYPAD_UP;
        case godot::Key::KEY_DOWN:
            return RETRO_DEVICE_ID_JOYPAD_DOWN;
        case godot::Key::KEY_LEFT:
            return RETRO_DEVICE_ID_JOYPAD_LEFT;
        case godot::Key::KEY_RIGHT:
            return RETRO_DEVICE_ID_JOYPAD_RIGHT;
        case godot::Key::KEY_Z:
            return RETRO_DEVICE_ID_JOYPAD_B;
        case godot::Key::KEY_X:
            return RETRO_DEVICE_ID_JOYPAD_A;
        case godot::Key::KEY_A:
            return RETRO_DEVICE_ID_JOYPAD_Y;
        case godot::Key::KEY_S:
            return RETRO_DEVICE_ID_JOYPAD_X;
        case godot::Key::KEY_ENTER:
            return RETRO_DEVICE_ID_JOYPAD_START;
        case godot::Key::KEY_SHIFT:
            return RETRO_DEVICE_ID_JOYPAD_SELECT;
        case godot::Key::KEY_Q:
            return RETRO_DEVICE_ID_JOYPAD_L;
        case godot::Key::KEY_W:
            return RETRO_DEVICE_ID_JOYPAD_R;
        default:
            return -1;
    }
}

void RetroHost::core_input_poll( void )
{
    // godot::UtilityFunctions::print( "Input poll" );
}

int16_t RetroHost::core_input_state( unsigned port, unsigned device, unsigned index, unsigned id )
{
    if ( port != 0 )
        return 0;

    switch ( device )
    {
        case RETRO_DEVICE_JOYPAD:
        {
            if ( id < 16 )
                return joypad_state[id] ? 1 : 0;
            return 0;
        }

        case RETRO_DEVICE_KEYBOARD:
        {
            if ( id < RETROK_LAST )
                return keyboard_state[id] ? 1 : 0;
            return 0;
        }
    }
    return 0;
}

void RetroHost::forwarded_input( const godot::Ref<godot::InputEvent> &event )
{
    if ( event->is_class( "InputEventKey" ) )
    {
        auto key_event = godot::Object::cast_to<godot::InputEventKey>( event.ptr() );

        if ( key_event->is_echo() )
            return;

        auto physical_key = key_event->get_physical_keycode();
        bool pressed = key_event->is_pressed();

        // Update joypad state from keyboard
        int joypad_btn = godotKeyToJoypadButton( physical_key );
        if ( joypad_btn >= 0 )
        {
            joypad_state[joypad_btn] = pressed;
        }

        // Update keyboard state
        auto retro_key = godotKeyToRetroKey( key_event->get_key_label() );

        if ( retro_key >= RETROK_LAST )
            return;

        // Forward keyboard event callback to core
        if ( this->core.retro_keyboard_event_callback )
        {
            uint16_t modifiers =
                ( RETROKMOD_ALT & ( key_event->is_alt_pressed() ? 0xFF : 0 ) ) |
                ( RETROKMOD_CTRL & ( key_event->is_ctrl_pressed() ? 0xFF : 0 ) ) |
                ( RETROKMOD_META & ( key_event->is_meta_pressed() ? 0xFF : 0 ) ) |
                ( RETROKMOD_SHIFT & ( key_event->is_shift_pressed() ? 0xFF : 0 ) );

            this->core.retro_keyboard_event_callback( pressed, retro_key, 0, modifiers );
        }

        keyboard_state[retro_key] = pressed;
    }
}
