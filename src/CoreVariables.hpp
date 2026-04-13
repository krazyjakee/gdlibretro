#pragma once

#include <map>
#include <string>
#include <vector>
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_string_array.hpp"
#include "godot_cpp/variant/string.hpp"
#include "libretro.h"
#include "yaml-cpp/yaml.h"

class CoreVariables
{
public:
    void load( const godot::String &core_name );
    void save();
    void cleanup();

    bool get_variable( retro_variable *variable );
    void set_variables( const retro_variable *variables );
    bool get_variable_update();

    godot::Dictionary get_all() const;
    void set( const godot::String &key, const godot::String &value );
    godot::PackedStringArray get_options( const godot::String &key ) const;

private:
    YAML::Node core_variables;
    godot::String core_name;
    bool variables_dirty = false;
    std::map<std::string, std::vector<std::string>> variable_options;
    std::map<std::string, char *> variable_string_cache;
};
