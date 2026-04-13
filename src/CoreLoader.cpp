#include "CoreLoader.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

// Platform-specific error handling for dynamic library loading
static std::string GetLastErrorAsStr()
{
#ifdef _WIN32
    DWORD errorMessageID = ::GetLastError();
    if ( errorMessageID == 0 )
    {
        return std::string();
    }

    LPSTR messageBuffer = nullptr;
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

// Platform-specific symbol loading
#ifdef _WIN32
#define GET_PROC_ADDRESS( handle, sym ) GetProcAddress( handle, sym )
#else
#define GET_PROC_ADDRESS( handle, sym ) dlsym( handle, sym )
#endif

#define load_symbol_return_false_on_err( handle, dest, sym )                                       \
    godot::UtilityFunctions::print( "[CoreLoader] Loading core symbol \"", #sym, "\"" );           \
    dest = (decltype( dest ))GET_PROC_ADDRESS( handle, #sym );                                     \
    if ( dest == NULL )                                                                            \
    {                                                                                              \
        godot::UtilityFunctions::printerr( "[CoreLoader] Could not load symbol \"", #sym,          \
                                           "\": ", GetLastErrorAsStr().c_str() );                   \
        return false;                                                                              \
    }

#define def_and_load_fn_symbol_return_false_on_err( handle, dest, sym )                            \
    godot::UtilityFunctions::print( "[CoreLoader] Loading core symbol \"", #sym, "\"" );           \
    decltype( sym ) *dest = reinterpret_cast<decltype( sym ) *>( GET_PROC_ADDRESS( handle, #sym ) ); \
    if ( dest == nullptr )                                                                         \
    {                                                                                              \
        godot::UtilityFunctions::printerr( "[CoreLoader] Could not load symbol \"", #sym,          \
                                           "\": ", GetLastErrorAsStr().c_str() );                   \
        return false;                                                                              \
    }

bool CoreLoader::load( const std::string &lib_path )
{
#ifdef _WIN32
    this->handle = LoadLibrary( lib_path.c_str() );
#else
    this->handle = dlopen( lib_path.c_str(), RTLD_LAZY );
#endif

    if ( this->handle == NULL )
    {
        godot::UtilityFunctions::print( "[CoreLoader] Failed to load core \"", lib_path.c_str(),
                                        "\"" );
        return false;
    }

    load_symbol_return_false_on_err( this->handle, this->retro_init, retro_init );
    load_symbol_return_false_on_err( this->handle, this->retro_deinit, retro_deinit );
    load_symbol_return_false_on_err( this->handle, this->retro_api_version, retro_api_version );
    load_symbol_return_false_on_err( this->handle, this->retro_get_system_info,
                                     retro_get_system_info );
    load_symbol_return_false_on_err( this->handle, this->retro_get_system_av_info,
                                     retro_get_system_av_info );
    load_symbol_return_false_on_err( this->handle, this->retro_set_controller_port_device,
                                     retro_set_controller_port_device );
    load_symbol_return_false_on_err( this->handle, this->retro_reset, retro_reset );
    load_symbol_return_false_on_err( this->handle, this->retro_run, retro_run );
    load_symbol_return_false_on_err( this->handle, this->retro_load_game, retro_load_game );
    load_symbol_return_false_on_err( this->handle, this->retro_unload_game, retro_unload_game );

    return true;
}

bool CoreLoader::set_callbacks( retro_environment_t env, retro_video_refresh_t video,
                                retro_input_poll_t poll, retro_input_state_t state,
                                retro_audio_sample_t sample, retro_audio_sample_batch_t batch )
{
    def_and_load_fn_symbol_return_false_on_err( this->handle, set_environment,
                                                retro_set_environment );
    def_and_load_fn_symbol_return_false_on_err( this->handle, set_video_refresh,
                                                retro_set_video_refresh );
    def_and_load_fn_symbol_return_false_on_err( this->handle, set_input_poll,
                                                retro_set_input_poll );
    def_and_load_fn_symbol_return_false_on_err( this->handle, set_input_state,
                                                retro_set_input_state );
    def_and_load_fn_symbol_return_false_on_err( this->handle, set_audio_sample,
                                                retro_set_audio_sample );
    def_and_load_fn_symbol_return_false_on_err( this->handle, set_audio_sample_batch,
                                                retro_set_audio_sample_batch );

    set_environment( env );
    set_video_refresh( video );
    set_input_poll( poll );
    set_input_state( state );
    set_audio_sample( sample );
    set_audio_sample_batch( batch );

    return true;
}

void CoreLoader::init()
{
    this->retro_init();
    this->initialized = true;
}

void CoreLoader::unload()
{
    if ( this->initialized )
    {
        godot::UtilityFunctions::print( "[CoreLoader] Deinitializing core" );
        this->retro_deinit();
        this->initialized = false;
    }
    if ( this->handle != NULL )
    {
        godot::UtilityFunctions::print( "[CoreLoader] Freeing core library" );
#ifdef _WIN32
        FreeLibrary( this->handle );
#else
        dlclose( this->handle );
#endif
        this->handle = NULL;
    }

    // Reset all function pointers
    this->retro_init = nullptr;
    this->retro_deinit = nullptr;
    this->retro_api_version = nullptr;
    this->retro_get_system_info = nullptr;
    this->retro_get_system_av_info = nullptr;
    this->retro_set_controller_port_device = nullptr;
    this->retro_reset = nullptr;
    this->retro_run = nullptr;
    this->retro_load_game = nullptr;
    this->retro_unload_game = nullptr;
    this->retro_keyboard_event_callback = nullptr;
}

godot::Dictionary CoreLoader::get_core_info( const std::string &lib_path )
{
    godot::Dictionary result;

#ifdef _WIN32
    HINSTANCE handle = LoadLibrary( lib_path.c_str() );
#else
    void *handle = dlopen( lib_path.c_str(), RTLD_LAZY );
#endif

    if ( handle == NULL )
    {
        godot::UtilityFunctions::printerr( "[CoreLoader] get_core_info: failed to load \"",
                                           lib_path.c_str(), "\"" );
        return result;
    }

    auto get_system_info = reinterpret_cast<void ( * )( struct retro_system_info * )>(
        GET_PROC_ADDRESS( handle, "retro_get_system_info" ) );

    if ( get_system_info == nullptr )
    {
        godot::UtilityFunctions::printerr(
            "[CoreLoader] get_core_info: could not find retro_get_system_info" );
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
    result["valid_extensions"] =
        godot::String( info.valid_extensions ? info.valid_extensions : "" );
    result["need_fullpath"] = info.need_fullpath;

#ifdef _WIN32
    FreeLibrary( handle );
#else
    dlclose( handle );
#endif

    return result;
}
