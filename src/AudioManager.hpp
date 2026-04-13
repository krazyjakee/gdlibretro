#pragma once

#include "godot_cpp/classes/audio_stream_generator.hpp"
#include "godot_cpp/classes/audio_stream_generator_playback.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "libretro.h"

class AudioManager
{
public:
    void set_audio_node( godot::Node *node );
    void init( const retro_system_av_info &av );
    void cleanup();
    void sample( int16_t left, int16_t right );
    size_t sample_batch( const int16_t *data, size_t frames );

private:
    godot::AudioStreamPlayer *audio_player = nullptr;
    godot::Node *user_audio_node = nullptr;
    bool owns_audio_player = false;
    godot::Ref<godot::AudioStreamGeneratorPlayback> audio_playback;
    godot::PackedVector2Array audio_batch_buffer;
    double audio_sample_rate = 0.0;
};
