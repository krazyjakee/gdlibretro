#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <string>
#include "godot_cpp/variant/dictionary.hpp"
#include "libretro.h"

class CoreLoader
{
public:
    bool load( const std::string &lib_path );
    void init();
    void unload();
    bool is_loaded() const { return initialized; }

    // Libretro function pointers
    void ( *retro_init )( void ) = nullptr;
    void ( *retro_deinit )( void ) = nullptr;
    unsigned ( *retro_api_version )( void ) = nullptr;
    void ( *retro_get_system_info )( struct retro_system_info *info ) = nullptr;
    void ( *retro_get_system_av_info )( struct retro_system_av_info *info ) = nullptr;
    void ( *retro_set_controller_port_device )( unsigned port, unsigned device ) = nullptr;
    void ( *retro_reset )( void ) = nullptr;
    void ( *retro_run )( void ) = nullptr;
    bool ( *retro_load_game )( const struct retro_game_info *game ) = nullptr;
    void ( *retro_unload_game )( void ) = nullptr;

    retro_keyboard_event_t retro_keyboard_event_callback = nullptr;

    // Set up libretro callbacks via the core's retro_set_* functions
    bool set_callbacks( retro_environment_t env, retro_video_refresh_t video,
                        retro_input_poll_t poll, retro_input_state_t state,
                        retro_audio_sample_t sample, retro_audio_sample_batch_t batch );

    // Static utility for querying core info without full load
    static godot::Dictionary get_core_info( const std::string &lib_path );

private:
#ifdef _WIN32
    HINSTANCE handle = NULL;
#else
    void *handle = nullptr;
#endif
    bool initialized = false;
};
