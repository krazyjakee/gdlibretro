#include "CoreVariables.hpp"
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

static godot::String get_variable_file_path( godot::String core_name )
{
    if ( godot::OS::get_singleton()->has_feature( "editor" ) )
    {
        return "res://libretro-cores/" + core_name + ".yml";
    }
    else
    {
        return "user://libretro-cores/" + core_name + ".yml";
    }
}

static std::vector<std::string> split( std::string s, std::string delimiter )
{
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ( ( pos_end = s.find( delimiter, pos_start ) ) != std::string::npos )
    {
        token = s.substr( pos_start, pos_end - pos_start );
        pos_start = pos_end + delim_len;
        res.push_back( token );
    }

    res.push_back( s.substr( pos_start ) );
    return res;
}

void CoreVariables::load( const godot::String &name )
{
    this->core_name = name;
    godot::String path = get_variable_file_path( core_name );

    auto file = godot::FileAccess::open( path, godot::FileAccess::ModeFlags::READ );
    if ( godot::FileAccess::get_open_error() != godot::Error::OK )
    {
        godot::UtilityFunctions::printerr(
            "[CoreVariables] Failed (", godot::FileAccess::get_open_error(),
            ") to open core variables file: ", path );
        this->core_variables = YAML::Node();
        return;
    }

    this->core_variables = YAML::Load( file->get_as_text().utf8().get_data() );
    file->close();
    godot::UtilityFunctions::print( "[CoreVariables] Loaded core variables file: ", path );
}

void CoreVariables::save()
{
    if ( !this->variables_dirty )
        return;

    godot::String path = get_variable_file_path( core_name );
    auto file = godot::FileAccess::open( path, godot::FileAccess::ModeFlags::WRITE_READ );
    if ( godot::FileAccess::get_open_error() != godot::Error::OK )
    {
        godot::UtilityFunctions::printerr(
            "[CoreVariables] Failed (", godot::FileAccess::get_open_error(),
            ") to save core variables file: ", path );
        return;
    }

    godot::UtilityFunctions::print( "[CoreVariables] Saving core variables file" );

    file->store_string( YAML::Dump( this->core_variables ).c_str() );
    file->close();
    this->variables_dirty = false;
}

void CoreVariables::cleanup()
{
    for ( auto &pair : this->variable_string_cache )
    {
        delete[] pair.second;
    }
    this->variable_string_cache.clear();
    this->variable_options.clear();
    this->variables_dirty = false;
}

bool CoreVariables::get_variable( retro_variable *variable )
{
    if ( !this->core_variables[variable->key].IsDefined() )
    {
        godot::UtilityFunctions::printerr( "[CoreVariables] Core variable ", variable->key,
                                           " not defined" );
        return false;
    }

    auto var_value = core_variables[variable->key].as<std::string>();
    if ( var_value.empty() )
    {
        godot::UtilityFunctions::printerr( "[CoreVariables] Core variable ", variable->key,
                                           " was empty ", var_value.c_str() );
        return false;
    }

    std::string cache_key = variable->key;
    auto it = this->variable_string_cache.find( cache_key );

    // Only allocate if the key is new or the value changed
    if ( it != this->variable_string_cache.end() )
    {
        if ( strcmp( it->second, var_value.c_str() ) == 0 )
        {
            variable->value = it->second;
            return true;
        }
        delete[] it->second;
    }

    const std::string::size_type size = var_value.size();
    char *buffer = new char[size + 1];
    memcpy( buffer, var_value.c_str(), size + 1 );

    this->variable_string_cache[cache_key] = buffer;
    variable->value = buffer;
    return true;
}

void CoreVariables::set_variables( const retro_variable *variables )
{
    while ( variables->key )
    {
        std::string value = variables->value;
        auto possible_values_str = split( value, ";" )[1].erase( 0, 1 );
        auto possible_values = split( possible_values_str, "|" );

        this->variable_options[variables->key] = possible_values;

        if ( !this->core_variables[variables->key].IsDefined() )
        {
            this->core_variables[variables->key] = possible_values[0];

            godot::UtilityFunctions::print(
                "[CoreVariables] Core variable ", variables->key,
                " was not present in the config file, now set to the first possible value: ",
                possible_values[0].c_str() );
        }
        variables++;
    }
}

bool CoreVariables::get_variable_update()
{
    bool dirty = this->variables_dirty;
    this->variables_dirty = false;
    return dirty;
}

godot::Dictionary CoreVariables::get_all() const
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

void CoreVariables::set( const godot::String &key, const godot::String &value )
{
    std::string k = key.utf8().get_data();
    std::string v = value.utf8().get_data();
    this->core_variables[k] = v;
    this->variables_dirty = true;
    godot::UtilityFunctions::print( "[CoreVariables] Set core variable ", key, " = ", value );
}

godot::PackedStringArray CoreVariables::get_options( const godot::String &key ) const
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
