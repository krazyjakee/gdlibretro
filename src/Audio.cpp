#include "RetroHost.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/vector2.hpp"

void RetroHost::core_audio_init( retro_system_av_info av )
{
    audio_sample_rate = av.timing.sample_rate;
    godot::UtilityFunctions::print( "[RetroHost] Audio init: sample rate = ", audio_sample_rate );

    // Create AudioStreamGenerator
    godot::Ref<godot::AudioStreamGenerator> generator;
    generator.instantiate();
    generator->set_mix_rate( audio_sample_rate );
    generator->set_buffer_length( 0.1 );

    // Create AudioStreamPlayer and add to scene tree
    audio_player = memnew( godot::AudioStreamPlayer );
    audio_player->set_stream( generator );

    auto *tree = godot::Object::cast_to<godot::SceneTree>(
        godot::Engine::get_singleton()->get_main_loop() );
    if ( tree && tree->get_root() )
    {
        tree->get_root()->add_child( audio_player );
    }
    else
    {
        godot::UtilityFunctions::printerr( "[RetroHost] Could not get scene tree for audio player" );
        return;
    }

    audio_player->play();
    audio_playback = audio_player->get_stream_playback();
}

static constexpr float INT16_TO_FLOAT = 1.0f / 32768.0f;

void RetroHost::core_audio_sample( int16_t left, int16_t right )
{
    if ( audio_playback.is_null() )
        return;

    if ( audio_playback->can_push_buffer( 1 ) )
    {
        audio_playback->push_frame(
            godot::Vector2( left * INT16_TO_FLOAT, right * INT16_TO_FLOAT ) );
    }
}

size_t RetroHost::core_audio_sample_batch( const int16_t *data, size_t frames )
{
    if ( audio_playback.is_null() )
        return frames;

    int available = audio_playback->get_frames_available();
    size_t to_push = frames < (size_t)available ? frames : (size_t)available;

    if ( to_push == 0 )
        return frames;

    audio_batch_buffer.resize( to_push );
    godot::Vector2 *buf = audio_batch_buffer.ptrw();

    for ( size_t i = 0; i < to_push; i++ )
    {
        buf[i] = godot::Vector2( data[i * 2] * INT16_TO_FLOAT,
                                 data[i * 2 + 1] * INT16_TO_FLOAT );
    }

    audio_playback->push_buffer( audio_batch_buffer );

    return frames;
}
