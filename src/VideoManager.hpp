#pragma once

#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "libretro.h"

class VideoManager
{
public:
    void init( const struct retro_game_geometry *geometry );
    void refresh( const void *data, unsigned width, unsigned height, size_t pitch );
    bool set_pixel_format( unsigned format );
    void cleanup();

    godot::Ref<godot::Image> get_frame_buffer() const { return frame_buffer; }

private:
    godot::Ref<godot::Image> frame_buffer;
    godot::PackedByteArray video_intermediary_buffer;
    unsigned pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
};
