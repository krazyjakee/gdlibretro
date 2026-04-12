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
- Linux: `demo/bin/LibRetroHost/lib/Linux-x86_64/libLibRetroHost.so`
- macOS: `demo/bin/LibRetroHost/lib/Darwin-Universal/libLibRetroHost.dylib`
- Windows: `demo/bin/LibRetroHost/lib/Windows-AMD64/LibRetroHost.dll`

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

Cores are extracted to `demo/libretro-cores/`.

**Requirements**: `7z` (p7zip-full) and `curl` or `wget`.

## Using a Core

1. Place your libretro core in `demo/libretro-cores/`
2. Modify `demo/node_3d.gd` to load your core:

```gdscript
# Load a core by name (without file extension)
RetroHost.load_core("your_core_libretro")
```

3. Run the demo project in Godot

## Project Structure

```
gdlibretro/
├── src/                    # LibRetroHost source code
├── godot-cpp/              # Godot C++ bindings (submodule)
├── yaml-cpp/               # YAML parsing library
├── libretro/               # Libretro API headers
├── demo/                   # Godot demo project
│   ├── bin/LibRetroHost/   # Compiled extension
│   └── libretro-cores/     # Place cores here
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
