#include "RetroHost.hpp"
#include "godot_cpp/classes/image.hpp"
#include "godot_cpp/variant/utility_functions.hpp"

void RetroHost::core_video_init( const struct retro_game_geometry *geometry )
{
    godot::UtilityFunctions::print( "[RetroHost] Video init ", geometry->base_width, " x ",
                                    geometry->base_height );
    this->frame_buffer = godot::Image::create( geometry->base_width, geometry->base_height, false,
                                               godot::Image::FORMAT_RGBA8 );
}

void RetroHost::core_video_refresh( const void *data, unsigned width, unsigned height,
                                    size_t pitch )
{
    if ( !data || frame_buffer.is_null() || !frame_buffer.is_valid() )
    {
        return;
    }

    if ( (unsigned)frame_buffer->get_width() != width ||
         (unsigned)frame_buffer->get_height() != height )
    {
        godot::UtilityFunctions::print( "[RetroHost] Resizing frame buffer to ", width, "x",
                                        height );
        auto created_frame_buffer =
            godot::Image::create( width, height, false, godot::Image::FORMAT_RGBA8 );
        if ( created_frame_buffer.is_null() || !created_frame_buffer.is_valid() )
        {
            godot::UtilityFunctions::printerr( "[RetroHost] Failed to recreate frame buffer" );
            return;
        }
        frame_buffer = created_frame_buffer;
    }

    unsigned dst_size = width * height * 4;
    if ( (unsigned)video_intermediary_buffer.size() != dst_size )
    {
        video_intermediary_buffer.resize( dst_size );
    }
    uint8_t *dst = video_intermediary_buffer.ptrw();
    const uint8_t *src = (const uint8_t *)data;

    switch ( this->core_pixel_format )
    {
        case RETRO_PIXEL_FORMAT_RGB565:
        {
            for ( unsigned y = 0; y < height; y++ )
            {
                const uint16_t *row = (const uint16_t *)( src + y * pitch );
                uint8_t *dst_row = dst + y * width * 4;
                for ( unsigned x = 0; x < width; x++ )
                {
                    uint16_t pixel = row[x];
                    uint8_t r = ( pixel >> 11 ) & 0x1F;
                    uint8_t g = ( pixel >> 5 ) & 0x3F;
                    uint8_t b = pixel & 0x1F;
                    dst_row[x * 4 + 0] = ( r << 3 ) | ( r >> 2 );
                    dst_row[x * 4 + 1] = ( g << 2 ) | ( g >> 4 );
                    dst_row[x * 4 + 2] = ( b << 3 ) | ( b >> 2 );
                    dst_row[x * 4 + 3] = 0xFF;
                }
            }
        }
        break;

        case RETRO_PIXEL_FORMAT_0RGB1555:
        {
            for ( unsigned y = 0; y < height; y++ )
            {
                const uint16_t *row = (const uint16_t *)( src + y * pitch );
                uint8_t *dst_row = dst + y * width * 4;
                for ( unsigned x = 0; x < width; x++ )
                {
                    uint16_t pixel = row[x];
                    uint8_t r = ( pixel >> 10 ) & 0x1F;
                    uint8_t g = ( pixel >> 5 ) & 0x1F;
                    uint8_t b = pixel & 0x1F;
                    dst_row[x * 4 + 0] = ( r << 3 ) | ( r >> 2 );
                    dst_row[x * 4 + 1] = ( g << 3 ) | ( g >> 2 );
                    dst_row[x * 4 + 2] = ( b << 3 ) | ( b >> 2 );
                    dst_row[x * 4 + 3] = 0xFF;
                }
            }
        }
        break;

        case RETRO_PIXEL_FORMAT_XRGB8888:
        {
            for ( unsigned y = 0; y < height; y++ )
            {
                const uint32_t *row = (const uint32_t *)( src + y * pitch );
                uint8_t *dst_row = dst + y * width * 4;
                for ( unsigned x = 0; x < width; x++ )
                {
                    uint32_t pixel = row[x];
                    dst_row[x * 4 + 0] = ( pixel >> 16 ) & 0xFF;
                    dst_row[x * 4 + 1] = ( pixel >> 8 ) & 0xFF;
                    dst_row[x * 4 + 2] = pixel & 0xFF;
                    dst_row[x * 4 + 3] = 0xFF;
                }
            }
        }
        break;

        default:
            godot::UtilityFunctions::printerr( "[RetroHost] Unhandled pixel format: ",
                                               this->core_pixel_format );
            return;
    }

    frame_buffer->set_data( width, height, false, godot::Image::FORMAT_RGBA8,
                            video_intermediary_buffer );
}

bool RetroHost::core_video_set_pixel_format( unsigned format )
{
    switch ( format )
    {
        case RETRO_PIXEL_FORMAT_0RGB1555:
            godot::UtilityFunctions::print( "[RetroHost] Pixel format: 0RGB1555" );
            this->core_pixel_format = RETRO_PIXEL_FORMAT_0RGB1555;
            return true;
        case RETRO_PIXEL_FORMAT_XRGB8888:
            godot::UtilityFunctions::print( "[RetroHost] Pixel format: XRGB8888" );
            this->core_pixel_format = RETRO_PIXEL_FORMAT_XRGB8888;
            return true;
        case RETRO_PIXEL_FORMAT_RGB565:
            godot::UtilityFunctions::print( "[RetroHost] Pixel format: RGB565" );
            this->core_pixel_format = RETRO_PIXEL_FORMAT_RGB565;
            return true;
        default:
            return false;
    }
}
