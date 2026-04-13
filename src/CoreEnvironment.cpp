#include "CoreEnvironment.hpp"
#include "RetroHost.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <cstdarg>

static void core_log( enum retro_log_level level, const char *fmt, ... )
{
    char buffer[4096] = { 0 };
    static const char *levelstr[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    va_list va;

    va_start( va, fmt );
    vsnprintf( buffer, sizeof( buffer ), fmt, va );
    va_end( va );

    godot::UtilityFunctions::print( "[RetroHost Loaded CORE][" +
                                    godot::String( levelstr[level - 1] ) + "] " + buffer );
}

bool core_environment( RetroHost &host, unsigned command, void *data )
{
    switch ( command )
    {
        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        {
            godot::UtilityFunctions::print( "[RetroHost] Core log interface set." );
            struct retro_log_callback *cb = (struct retro_log_callback *)data;
            cb->log = core_log;
        }
        break;

        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        {
            godot::UtilityFunctions::print( "[RetroHost] Core can dupe set." );
            bool *b = (bool *)data;
            *b = true;
        }
        break;

        case RETRO_ENVIRONMENT_GET_VARIABLE:
        {
            auto var = (retro_variable *)data;
            return host.variables.get_variable( var );
        }
        break;

        case RETRO_ENVIRONMENT_SET_VARIABLES:
        {
            auto variables = (const struct retro_variable *)data;
            host.variables.set_variables( variables );
        }
        break;

        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        {
            bool *b = (bool *)data;
            *b = host.variables.get_variable_update();
        }
        break;

        case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
        {
            auto vfs_interface = (struct retro_vfs_interface_info *)data;
            godot::UtilityFunctions::print( "[RetroHost] Core requested VFS interface" );
            if ( vfs_interface->required_interface_version >
                 host.vfs.supported_interface_version )
            {
                godot::UtilityFunctions::printerr(
                    "[RetroHost] Core requested VFS interface v",
                    vfs_interface->required_interface_version, " we only support up to v",
                    host.vfs.supported_interface_version );
                return false;
            }
            godot::UtilityFunctions::print( "[RetroHost] Core requested VFS interface v",
                                            vfs_interface->required_interface_version );
            vfs_interface->iface = &host.vfs.vfs_interface;
            return true;
        }
        break;

        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        {
            unsigned *version = (unsigned *)data;
            *version = 0;
            return false;
        }

        case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
        {
            unsigned *version = (unsigned *)data;
            *version = 0;
            return false;
        }
        break;

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        {
            const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;

            if ( *fmt > RETRO_PIXEL_FORMAT_RGB565 )
            {
                return false;
            }

            godot::UtilityFunctions::print( "[RetroHost] Core setting pixel format" );
            return host.video.set_pixel_format( *fmt );
        }
        break;

        case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
        {
            auto callback = (const struct retro_keyboard_callback *)data;
            host.core.retro_keyboard_event_callback = callback->callback;
            godot::UtilityFunctions::print( "[RetroHost] keyboard callback set" );
        }
        break;

        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        {
            if ( !data )
                return false;
            bool *b = (bool *)data;
            *b = false;
        }
        break;

        case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS:
        {
            const struct retro_input_descriptor *desc =
                (const struct retro_input_descriptor *)data;
            while ( desc->description )
            {
                godot::UtilityFunctions::print( "[RetroHost] Core input descriptor: ",
                                                desc->description );
                desc++;
            }
        }
        break;

        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        {
            bool *b = (bool *)data;
            if ( *b )
            {
                godot::UtilityFunctions::print( "[RetroHost] Core supports no game" );
            }
            else
            {
                godot::UtilityFunctions::print( "[RetroHost] Core does not support no game" );
            }
        }
        break;

        case RETRO_ENVIRONMENT_GET_THROTTLE_STATE:
        {
            auto throttle_state = (struct retro_throttle_state *)data;
            throttle_state->mode = RETRO_THROTTLE_UNBLOCKED;
            throttle_state->rate = 0;
            return true;
        }
        break;

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:
        case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
        {
            godot::UtilityFunctions::print( "[RetroHost] Core requested path" );
            host.cwd_path_cache = host.cwd.trim_suffix( "/" ).utf8().get_data();
            *(const char **)data = host.cwd_path_cache.c_str();
            return true;
        }

        case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
        {
            if ( !data )
                return false;
            bool *b = (bool *)data;
            *b = false;
        }
        break;

        case RETRO_ENVIRONMENT_SHUTDOWN:
        {
            godot::UtilityFunctions::print( "[RetroHost] Core shutdown requested" );
            break;
        }

        default:
            return false;
    }

    return true;
}
