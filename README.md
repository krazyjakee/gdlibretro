# GDLibretro

A GDExtension that provides libretro core hosting for Godot 4.5+.

## Overview

GDLibretro allows you to load and run libretro cores (the same cores used by RetroArch) directly within your Godot project. This enables emulation, game engines, and other libretro-compatible software to run inside Godot.

## Requirements

- Godot 4.5+
- SCons
- Platform-specific compiler:
  - **Linux**: GCC/G++
  - **macOS**: Clang (Xcode Command Line Tools)
  - **Windows (cross-compile)**: MinGW-w64 (`x86_64-w64-mingw32-gcc`)
  - **Windows (native)**: MSVC

## Building

### Quick Start

```bash
# Clone with submodules
git clone --recursive https://github.com/krazyjakee/gdlibretro.git
cd gdlibretro

# Build for your platform
./build.sh linux      # Linux
./build.sh macos      # macOS
./build.sh windows    # Windows (cross-compile from Linux)
```

### Build Options

```bash
./build.sh <platform> [options]

Platforms:
  linux     Build for Linux (x86_64)
  macos     Build for macOS (universal)
  windows   Build for Windows (x86_64, cross-compile with MinGW)

Options:
  --debug   Build debug version (default: release)
```

### Build Output

Libraries are placed in:
- Linux: `addons/gdlibretro/lib/Linux-x86_64/gdlibretro.so`
- macOS: `addons/gdlibretro/lib/Darwin-Universal/gdlibretro.dylib`
- Windows: `addons/gdlibretro/lib/Windows-AMD64/gdlibretro.dll`

### Building Manually with SCons

If you prefer to invoke SCons directly:

```bash
# Linux
scons platform=linux target=template_release

# macOS
scons platform=macos arch=universal target=template_release

# Windows (cross-compile)
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++
scons platform=windows use_mingw=yes target=template_release
```

## Testing

After building, verify the extension loads correctly:

```bash
./test.sh
```

This script:
1. Checks for the compiled library
2. Imports the demo project in Godot (headless)
3. Runs the demo scene briefly to verify functionality

## Downloading Libretro Cores

Use `cores.sh` to download all RetroArch cores for your platform:

```bash
./cores.sh linux    # Download Linux cores
./cores.sh windows  # Download Windows cores
```

Cores are extracted to `libretro-cores/`.

**Requirements**: `7z` (p7zip-full) and `curl` or `wget`.

## RetroHost API

GDLibretro exposes a `RetroHost` singleton that you can call from any GDScript. Place your libretro core libraries in `libretro-cores/`.

### Core Lifecycle

```gdscript
# Load a core by name (without file extension)
RetroHost.load_core("genesis_plus_gx_libretro")

# Load a ROM/game file
RetroHost.load_game("/path/to/rom.md")

# Run one frame — call this every frame in _process()
RetroHost.run()

# Unload the current core and game
RetroHost.unload_core()
```

### Video

```gdscript
# Get the current frame as a Godot Image (returns null if no frame is ready)
var frame: Image = RetroHost.get_frame_buffer()

# Display it on a TextureRect
if frame:
    my_texture_rect.texture = ImageTexture.create_from_image(frame)
```

### Audio

Audio playback is automatic — when a game is loaded, RetroHost creates an internal `AudioStreamPlayer` and pushes samples from the core.

To use your own audio node instead (e.g. `AudioStreamPlayer3D` for spatial audio), call `set_audio_node` before loading a game:

```gdscript
@onready var audio: AudioStreamPlayer3D = $AudioStreamPlayer3D

func _ready():
    RetroHost.set_audio_node(audio)
    RetroHost.load_core("genesis_plus_gx_libretro")
    RetroHost.load_game("res://roms/sonic.md")
```

`set_audio_node` accepts any `AudioStreamPlayer`, `AudioStreamPlayer2D`, or `AudioStreamPlayer3D`. RetroHost will assign an `AudioStreamGenerator` stream and manage playback automatically.

### Input

```gdscript
# Forward Godot input events to the core (call from _input)
func _input(event):
    RetroHost.forward_input(event)
```

Keyboard events are mapped to a default joypad layout (WASD + arrow keys for D-pad, Z/X for buttons, etc.).

### Core Info

```gdscript
# Get metadata about a core without fully loading it
var info: Dictionary = RetroHost.get_core_info("genesis_plus_gx_libretro")
# Returns: { "library_name": "Genesis Plus GX", "library_version": "1.7.4",
#             "valid_extensions": "mdx|md|smd|gen|bin|...", "need_fullpath": false }
```

### Core Variables (Settings)

```gdscript
# Get all current core variables as { key: current_value }
var variables: Dictionary = RetroHost.get_core_variables()

# Get the list of valid options for a variable
var options: PackedStringArray = RetroHost.get_core_variable_options("genesis_plus_gx_bram")

# Set a core variable
RetroHost.set_core_variable("genesis_plus_gx_bram", "per game")
```

### Minimal Example

```gdscript
extends Node

@onready var display: TextureRect = $TextureRect

func _ready():
    RetroHost.load_core("genesis_plus_gx_libretro")
    RetroHost.load_game("res://roms/sonic.md")

func _process(_delta):
    RetroHost.run()
    var frame = RetroHost.get_frame_buffer()
    if frame:
        display.texture = ImageTexture.create_from_image(frame)

func _input(event):
    RetroHost.forward_input(event)
```

## Project Structure

```
gdlibretro/
├── src/                    # gdlibretro source code
├── godot-cpp/              # Godot C++ bindings (submodule)
├── yaml-cpp/               # YAML parsing library
├── libretro/               # Libretro API headers
├── libretro-cores/         # Place cores here
├── addons/                 # Godot demo project
│   ├── gdlibretro/         # Compiled extension
├── build.sh                # Build script
├── test.sh                 # Test script
├── cores.sh                # Core downloader
└── SConstruct              # SCons build configuration
```

## Debugging (VSCode)

1. Open `.vscode/launch.json`
2. Update the Godot executable path in the launch configuration
3. Build with `--debug` flag: `./build.sh linux --debug`
4. Start debugging in VSCode

## Exported Projects

The extension works with exported Godot projects. Place your core libraries where the OS can find them:
- Same directory as the executable
- System library paths

## Notes

- This is a hobby project
- If you encounter issues, feel free to open an issue
- The "stack destroyed" exception with some debug cores can be safely ignored
