#include "AudioManager.hpp"
#include "godot_cpp/classes/engine.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/window.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/vector2.hpp"

void AudioManager::set_audio_node( godot::Node *node )
{
    user_audio_node = node;
}

void AudioManager::init( const retro_system_av_info &av )
{
    audio_sample_rate = av.timing.sample_rate;
    godot::UtilityFunctions::print( "[AudioManager] Audio init: sample rate = ", audio_sample_rate );

    godot::Ref<godot::AudioStreamGenerator> generator;
    generator.instantiate();
    generator->set_mix_rate( audio_sample_rate );
    generator->set_buffer_length( 0.1 );

    if ( user_audio_node )
    {
        user_audio_node->call( "set_stream", generator );
        user_audio_node->call( "play" );
        audio_playback = user_audio_node->call( "get_stream_playback" );
        owns_audio_player = false;
    }
    else
    {
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
            godot::UtilityFunctions::printerr(
                "[AudioManager] Could not get scene tree for audio player" );
            return;
        }

        audio_player->play();
        audio_playback = audio_player->get_stream_playback();
        owns_audio_player = true;
    }
}

void AudioManager::cleanup()
{
    if ( this->user_audio_node )
    {
        this->user_audio_node->call( "stop" );
        this->user_audio_node = nullptr;
    }
    if ( this->audio_player && this->owns_audio_player )
    {
        auto *tree = godot::Object::cast_to<godot::SceneTree>(
            godot::Engine::get_singleton()->get_main_loop() );
        if ( tree && tree->get_root() )
        {
            this->audio_player->stop();
            this->audio_player->queue_free();
        }
    }
    this->audio_player = nullptr;
    this->owns_audio_player = false;
    this->audio_playback = godot::Ref<godot::AudioStreamGeneratorPlayback>();
}

static constexpr float INT16_TO_FLOAT = 1.0f / 32768.0f;

void AudioManager::sample( int16_t left, int16_t right )
{
    if ( audio_playback.is_null() )
        return;

    if ( audio_playback->can_push_buffer( 1 ) )
    {
        audio_playback->push_frame(
            godot::Vector2( left * INT16_TO_FLOAT, right * INT16_TO_FLOAT ) );
    }
}

size_t AudioManager::sample_batch( const int16_t *data, size_t frames )
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
