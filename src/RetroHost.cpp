#include "RetroHost.hpp"
#include "CoreEnvironment.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>

RetroHost::RetroHost()
{
    godot::UtilityFunctions::print( "[RetroHost] Constructor" );
    singleton = this;
    this->vfs.init_vfs_interface();
}

RetroHost::~RetroHost()
{
    godot::UtilityFunctions::print( "[RetroHost] Destructor" );
    this->unload_core();
}

RetroHost *RetroHost::singleton = nullptr;

RetroHost *RetroHost::get_singleton()
{
    return singleton;
}

bool RetroHost::load_core( godot::String name )
{
    this->unload_core();
    godot::UtilityFunctions::print( "[RetroHost] Loading core \"", name, "\"" );

    godot::String lib_path;

    // Platform-specific library extension
#ifdef _WIN32
    const char *lib_ext = ".dll";
#elif defined( __APPLE__ )
    const char *lib_ext = ".dylib";
#else
    const char *lib_ext = ".so";
#endif

    if ( godot::OS::get_singleton()->has_feature( "editor" ) )
    {
        this->cwd =
            godot::ProjectSettings::get_singleton()->globalize_path( "res://" ) + "libretro-cores/";
        lib_path = cwd + name + lib_ext;
    }
    else
    {
        this->cwd = godot::OS::get_singleton()->get_executable_path().get_base_dir();
        lib_path = name;
    }

    if ( !this->core.load( lib_path.utf8().get_data() ) )
    {
        godot::UtilityFunctions::print( "[RetroHost] Failed to load core \"", lib_path, "\"" );
        return false;
    }

    this->variables.load( name );

    // Set up libretro callbacks via lambdas that route through the singleton
    bool callbacks_ok = this->core.set_callbacks(
        []( unsigned cmd, void *data ) {
            return core_environment( *get_singleton(), cmd, data );
        },
        []( const void *data, unsigned width, unsigned height, size_t pitch ) {
            get_singleton()->video.refresh( data, width, height, pitch );
        },
        []( void ) { get_singleton()->input.poll(); },
        []( unsigned port, unsigned device, unsigned index, unsigned id ) {
            return get_singleton()->input.state( port, device, index, id );
        },
        []( int16_t left, int16_t right ) { get_singleton()->audio.sample( left, right ); },
        []( const int16_t *data, size_t frames ) {
            return get_singleton()->audio.sample_batch( data, frames );
        } );

    if ( !callbacks_ok )
    {
        godot::UtilityFunctions::printerr( "[RetroHost] Failed to set callbacks" );
        return false;
    }

    this->core.init();

    return true;
}

bool RetroHost::load_game( godot::String path )
{
    if ( !this->core.is_loaded() )
    {
        godot::UtilityFunctions::printerr( "[RetroHost] Cannot load game: core not initialized" );
        return false;
    }

    struct retro_game_info game_info = { 0 };
    std::vector<uint8_t> file_data;

    if ( path.is_empty() )
    {
        godot::UtilityFunctions::print( "[RetroHost] Loading game with no content (no-game mode)" );
        if ( !this->core.retro_load_game( NULL ) )
        {
            godot::UtilityFunctions::printerr( "[RetroHost] retro_load_game(NULL) failed" );
            return false;
        }
    }
    else
    {
        godot::UtilityFunctions::print( "[RetroHost] Loading game: ", path );

        auto file = godot::FileAccess::open( path, godot::FileAccess::ModeFlags::READ );
        if ( godot::FileAccess::get_open_error() != godot::Error::OK )
        {
            godot::UtilityFunctions::printerr( "[RetroHost] Failed to open game file: ", path );
            return false;
        }

        int64_t file_size = file->get_length();
        godot::PackedByteArray bytes = file->get_buffer( file_size );
        file->close();

        file_data.resize( file_size );
        memcpy( file_data.data(), bytes.ptr(), file_size );

        auto path_utf8 = path.utf8();
        game_info.path = path_utf8.get_data();
        game_info.data = file_data.data();
        game_info.size = file_size;

        if ( !this->core.retro_load_game( &game_info ) )
        {
            godot::UtilityFunctions::printerr( "[RetroHost] retro_load_game failed for: ", path );
            return false;
        }
    }

    struct retro_system_av_info av;
    this->core.retro_get_system_av_info( &av );

    this->video.init( &av.geometry );
    this->audio.init( av );

    this->game_loaded = true;

    this->core.retro_run();

    return true;
}

void RetroHost::unload_core()
{
    // Clean up audio
    this->audio.cleanup();

    // Clean up variables (free cached strings)
    this->variables.cleanup();

    if ( this->game_loaded )
    {
        this->core.retro_unload_game();
        this->game_loaded = false;
    }
    if ( this->core.is_loaded() )
    {
        this->variables.save();
    }

    // Unload core library
    this->core.unload();
}

void RetroHost::run()
{
    if ( !this->core.is_loaded() || !this->game_loaded )
    {
        return;
    }
    this->core.retro_run();
}

void RetroHost::forwarded_input( const godot::Ref<godot::InputEvent> &event )
{
    this->input.forward_event( event, this->core.retro_keyboard_event_callback );
}

void RetroHost::set_audio_node( godot::Node *node )
{
    this->audio.set_audio_node( node );
}

godot::Dictionary RetroHost::get_core_variables()
{
    return this->variables.get_all();
}

void RetroHost::set_core_variable( godot::String key, godot::String value )
{
    this->variables.set( key, value );
}

godot::PackedStringArray RetroHost::get_core_variable_options( godot::String key )
{
    return this->variables.get_options( key );
}

godot::Dictionary RetroHost::get_core_info( godot::String name )
{
    godot::String lib_path;

#ifdef _WIN32
    const char *lib_ext = ".dll";
#elif defined( __APPLE__ )
    const char *lib_ext = ".dylib";
#else
    const char *lib_ext = ".so";
#endif

    if ( godot::OS::get_singleton()->has_feature( "editor" ) )
    {
        godot::String cores_dir =
            godot::ProjectSettings::get_singleton()->globalize_path( "res://" ) + "libretro-cores/";
        lib_path = cores_dir + name + lib_ext;
    }
    else
    {
        lib_path = name;
    }

    return CoreLoader::get_core_info( lib_path.utf8().get_data() );
}

void RetroHost::_bind_methods()
{
    godot::ClassDB::bind_method( godot::D_METHOD( "load_core", "name" ), &RetroHost::load_core );
    godot::ClassDB::bind_method( godot::D_METHOD( "load_game", "path" ), &RetroHost::load_game );
    godot::ClassDB::bind_method( godot::D_METHOD( "unload_core" ), &RetroHost::unload_core );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_info", "name" ),
                                 &RetroHost::get_core_info );
    godot::ClassDB::bind_method( godot::D_METHOD( "run" ), &RetroHost::run );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_frame_buffer" ),
                                 &RetroHost::get_frame_buffer );
    godot::ClassDB::bind_method( godot::D_METHOD( "forward_input", "event" ),
                                 &RetroHost::forwarded_input );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_audio_node", "node" ),
                                 &RetroHost::set_audio_node );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_variables" ),
                                 &RetroHost::get_core_variables );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_core_variable", "key", "value" ),
                                 &RetroHost::set_core_variable );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_variable_options", "key" ),
                                 &RetroHost::get_core_variable_options );
}
