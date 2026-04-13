#pragma once

#include "libretro.h"
#include "libretro-common/include/vfs/vfs_implementation.h"

class RetroHost; // forward declare for singleton access in init_vfs_interface

class VFS
{
public:
    uint32_t supported_interface_version = 0;

    void init_vfs_interface();
    struct retro_vfs_interface vfs_interface;

    // VFS API v1
    const char *get_path( retro_vfs_file_handle *stream );
    struct retro_vfs_file_handle *open( const char *path, unsigned mode, unsigned hints );
    int close( retro_vfs_file_handle *stream );
    int64_t size( struct retro_vfs_file_handle *stream );
    int64_t tell( struct retro_vfs_file_handle *stream );
    int64_t seek( struct retro_vfs_file_handle *stream, int64_t offset, int seek_position );
    int64_t read( struct retro_vfs_file_handle *stream, void *s, uint64_t len );
    int64_t write( struct retro_vfs_file_handle *stream, const void *s, uint64_t len );
    int flush( struct retro_vfs_file_handle *stream );
    int remove( const char *path );
    int rename( const char *old_path, const char *new_path );

    // VFS API v2
    int64_t truncate( struct retro_vfs_file_handle *stream, int64_t length );

    // VFS API v3
    int stat( const char *path, int32_t *size );
    int mkdir( const char *dir );
    struct retro_vfs_dir_handle *opendir( const char *dir, bool include_hidden_files );
    bool read_dir( struct retro_vfs_dir_handle *dir_stream );
    const char *dirent_get_name( struct retro_vfs_dir_handle *dir_stream );
    bool dirent_is_dir( struct retro_vfs_dir_handle *dir_stream );
    int closedir( struct retro_vfs_dir_handle *dir_stream );
};
