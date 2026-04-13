#pragma once

#include <string>
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"

#include "AudioManager.hpp"
#include "CoreLoader.hpp"
#include "CoreVariables.hpp"
#include "InputManager.hpp"
#include "VFS.hpp"
#include "VideoManager.hpp"

class RetroHost : public godot::Object
{
    GDCLASS( RetroHost, godot::Object )

public:
    godot::String cwd;
    static RetroHost *get_singleton();

    RetroHost();
    ~RetroHost();

    bool load_core( godot::String path );
    bool load_game( godot::String path );
    void unload_core();
    godot::Dictionary get_core_info( godot::String name );
    void run();
    void forwarded_input( const godot::Ref<godot::InputEvent> &event );
    void set_audio_node( godot::Node *node );
    godot::Dictionary get_core_variables();
    void set_core_variable( godot::String key, godot::String value );
    godot::PackedStringArray get_core_variable_options( godot::String key );

    // Subsystems (public so CoreEnvironment can access them)
    CoreLoader core;
    AudioManager audio;
    VideoManager video;
    InputManager input;
    CoreVariables variables;
    VFS vfs;

    // Cached cwd string for returning to cores (avoids dangling pointer from temporaries)
    std::string cwd_path_cache;

private:
    static RetroHost *singleton;
    godot::Ref<godot::Image> get_frame_buffer() { return video.get_frame_buffer(); }
    bool game_loaded = false;

protected:
    static void _bind_methods();
};
