#include "RetroHost.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>

#include <fstream>
#include <iostream>

// Platform-specific error handling for dynamic library loading
std::string GetLastErrorAsStr()
{
#ifdef _WIN32
    // Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if ( errorMessageID == 0 )
    {
        return std::string(); // No error message has been recorded
    }

    LPSTR messageBuffer = nullptr;

    // Ask Win32 to give us the string version of that message ID.
    size_t size = FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                      FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL, errorMessageID, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                  (LPSTR)&messageBuffer, 0, NULL );

    std::string message( messageBuffer, size );
    LocalFree( messageBuffer );

    return message;
#else
    const char *error = dlerror();
    return error ? std::string( error ) : std::string();
#endif
}

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

// Platform-specific symbol loading
#ifdef _WIN32
#define GET_PROC_ADDRESS( handle, sym ) GetProcAddress( handle, sym )
#else
#define GET_PROC_ADDRESS( handle, sym ) dlsym( handle, sym )
#endif

#define load_symbol_return_false_on_err( handle, dest, sym )                                       \
    godot::UtilityFunctions::print( "[RetroHost] Loading core symbol \"", #sym, "\"" );            \
    dest = (decltype( dest ))GET_PROC_ADDRESS( handle, #sym );                                     \
    if ( dest == NULL )                                                                            \
    {                                                                                              \
        godot::UtilityFunctions::printerr( "[RetroHost] Could not load symbol \"", #sym,           \
                                           "\": ", GetLastErrorAsStr().c_str() );                  \
        return false;                                                                              \
    }

#define def_and_load_fn_symbol_return_false_on_err( handle, dest, sym )                            \
    godot::UtilityFunctions::print( "[RetroHost] Loading core symbol \"", #sym, "\"" );            \
    decltype( sym ) *dest = reinterpret_cast<decltype( sym ) *>( GET_PROC_ADDRESS( handle, #sym ) ); \
    if ( dest == nullptr )                                                                         \
    {                                                                                              \
        godot::UtilityFunctions::printerr( "[RetroHost] Could not load symbol \"", #sym,           \
                                           "\": ", GetLastErrorAsStr().c_str() );                  \
        return false;                                                                              \
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
        // MAYBE TODO: Add a setting to change the core base path in the editor
        // the current working directory in the editor is the godot executable itself, but we want
        // to load the cores from the project directory
        this->cwd =
            godot::ProjectSettings::get_singleton()->globalize_path( "res://" ) + "libretro-cores/";
        lib_path = cwd + name + lib_ext;
    }
    else
    {
        // OS can handle loading libraries by name alone if we're in an exported build
        this->cwd = godot::OS::get_singleton()->get_executable_path().get_base_dir();
        lib_path = name;
    }

#ifdef _WIN32
    this->core.handle = LoadLibrary( lib_path.utf8().get_data() );
#else
    this->core.handle = dlopen( lib_path.utf8().get_data(), RTLD_LAZY );
#endif

    if ( this->core.handle == NULL )
    {
        godot::UtilityFunctions::print( "[RetroHost] Failed to load core \"", lib_path, "\"" );
        return false;
    }

    load_symbol_return_false_on_err( this->core.handle, this->core.retro_init, retro_init );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_deinit, retro_deinit );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_api_version,
                                     retro_api_version );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_get_system_info,
                                     retro_get_system_info );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_get_system_av_info,
                                     retro_get_system_av_info );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_set_controller_port_device,
                                     retro_set_controller_port_device );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_reset, retro_reset );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_run, retro_run );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_load_game,
                                     retro_load_game );
    load_symbol_return_false_on_err( this->core.handle, this->core.retro_unload_game,
                                     retro_unload_game );

    def_and_load_fn_symbol_return_false_on_err( this->core.handle, set_environment,
                                                retro_set_environment );
    def_and_load_fn_symbol_return_false_on_err( this->core.handle, set_video_refresh,
                                                retro_set_video_refresh );
    def_and_load_fn_symbol_return_false_on_err( this->core.handle, set_input_poll,
                                                retro_set_input_poll );
    def_and_load_fn_symbol_return_false_on_err( this->core.handle, set_input_state,
                                                retro_set_input_state );
    def_and_load_fn_symbol_return_false_on_err( this->core.handle, set_audio_sample,
                                                retro_set_audio_sample );
    def_and_load_fn_symbol_return_false_on_err( this->core.handle, set_audio_sample_batch,
                                                retro_set_audio_sample_batch );

    this->core_name = name;
    this->load_core_variables();

    // May god have mercy on our souls for the crimes we are about to commit
    // Blame c++

    set_environment(
        []( unsigned cmd, void *data ) { return get_singleton()->core_environment( cmd, data ); } );

    set_video_refresh( []( const void *data, unsigned width, unsigned height, size_t pitch ) {
        get_singleton()->core_video_refresh( data, width, height, pitch );
    } );

    set_input_poll( []( void ) { get_singleton()->core_input_poll(); } );

    set_input_state( []( unsigned port, unsigned device, unsigned index, unsigned id ) {
        return get_singleton()->core_input_state( port, device, index, id );
    } );

    set_audio_sample(
        []( int16_t left, int16_t right ) { get_singleton()->core_audio_sample( left, right ); } );

    set_audio_sample_batch( []( const int16_t *data, size_t frames ) {
        return get_singleton()->core_audio_sample_batch( data, frames );
    } );

    // End of c++ crimes

    this->core.retro_init();
    this->core.initialized = true;

    return true;
}

bool RetroHost::load_game( godot::String path )
{
    if ( !this->core.initialized )
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

        // Keep the CharString alive until after retro_load_game
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

    this->core_video_init( &av.geometry );
    this->core_audio_init( av );

    this->game_loaded = true;

    this->core.retro_run();

    return true;
}

void RetroHost::unload_core()
{
    // Clean up audio (only if scene tree is still alive, not during engine shutdown)
    if ( this->audio_player )
    {
        auto *tree = godot::Object::cast_to<godot::SceneTree>(
            godot::Engine::get_singleton()->get_main_loop() );
        if ( tree && tree->get_root() )
        {
            this->audio_player->stop();
            this->audio_player->queue_free();
        }
        this->audio_player = nullptr;
    }
    this->audio_playback = godot::Ref<godot::AudioStreamGeneratorPlayback>();

    // Free all the strings we've allocated for the core
    for ( auto &pair : this->variable_string_cache )
    {
        delete[] pair.second;
    }
    this->variable_string_cache.clear();
    if ( this->game_loaded )
    {
        this->core.retro_unload_game();
        this->game_loaded = false;
    }
    if ( this->core.initialized )
    {
        godot::UtilityFunctions::print( "[RetroHost] Deinitializing core" );
        this->core.retro_deinit();
        this->core.initialized = false;
        this->save_core_variables();
    }
    this->variable_options.clear();
    if ( this->core.handle != NULL )
    {
        godot::UtilityFunctions::print( "[RetroHost] Freeing core library" );
#ifdef _WIN32
        FreeLibrary( this->core.handle );
#else
        dlclose( this->core.handle );
#endif
        this->core.handle = NULL;
    }
}

void RetroHost::run()
{
    if ( !this->core.initialized || !this->game_loaded )
    {
        return;
    }
    this->core.retro_run();
}

godot::Dictionary RetroHost::get_core_variables()
{
    godot::Dictionary result;
    for ( auto it = this->core_variables.begin(); it != this->core_variables.end(); ++it )
    {
        godot::String key = godot::String( it->first.as<std::string>().c_str() );
        godot::String value = godot::String( it->second.as<std::string>().c_str() );
        result[key] = value;
    }
    return result;
}

void RetroHost::set_core_variable( godot::String key, godot::String value )
{
    std::string k = key.utf8().get_data();
    std::string v = value.utf8().get_data();
    this->core_variables[k] = v;
    this->variables_dirty = true;
    godot::UtilityFunctions::print( "[RetroHost] Set core variable ", key, " = ", value );
}

godot::PackedStringArray RetroHost::get_core_variable_options( godot::String key )
{
    godot::PackedStringArray result;
    std::string k = key.utf8().get_data();
    auto it = this->variable_options.find( k );
    if ( it != this->variable_options.end() )
    {
        for ( const auto &opt : it->second )
        {
            result.push_back( godot::String( opt.c_str() ) );
        }
    }
    return result;
}

godot::Dictionary RetroHost::get_core_info( godot::String name )
{
    godot::Dictionary result;

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

#ifdef _WIN32
    HINSTANCE handle = LoadLibrary( lib_path.utf8().get_data() );
#else
    void *handle = dlopen( lib_path.utf8().get_data(), RTLD_LAZY );
#endif

    if ( handle == NULL )
    {
        godot::UtilityFunctions::printerr( "[RetroHost] get_core_info: failed to load \"", lib_path,
                                           "\"" );
        return result;
    }

    auto get_system_info = reinterpret_cast<void ( * )( struct retro_system_info * )>(
        GET_PROC_ADDRESS( handle, "retro_get_system_info" ) );

    if ( get_system_info == nullptr )
    {
        godot::UtilityFunctions::printerr(
            "[RetroHost] get_core_info: could not find retro_get_system_info in \"", name, "\"" );
#ifdef _WIN32
        FreeLibrary( handle );
#else
        dlclose( handle );
#endif
        return result;
    }

    struct retro_system_info info = {};
    get_system_info( &info );

    result["library_name"] = godot::String( info.library_name ? info.library_name : "" );
    result["library_version"] = godot::String( info.library_version ? info.library_version : "" );
    result["valid_extensions"] = godot::String( info.valid_extensions ? info.valid_extensions : "" );
    result["need_fullpath"] = info.need_fullpath;

#ifdef _WIN32
    FreeLibrary( handle );
#else
    dlclose( handle );
#endif

    return result;
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
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_variables" ),
                                 &RetroHost::get_core_variables );
    godot::ClassDB::bind_method( godot::D_METHOD( "set_core_variable", "key", "value" ),
                                 &RetroHost::set_core_variable );
    godot::ClassDB::bind_method( godot::D_METHOD( "get_core_variable_options", "key" ),
                                 &RetroHost::get_core_variable_options );
}