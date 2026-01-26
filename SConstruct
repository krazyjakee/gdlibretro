#!/usr/bin/env python

from SCons.Script import ARGUMENTS, Environment, Mkdir, Default, File, CacheDir, SConscript
import os
import sys

# Project configuration
PROJECT_NAME = "LibRetroHost"

# Resolve platform/target/arch from args (matches CI defaults)
platform = ARGUMENTS.get('platform')
if not platform:
    p = sys.platform
    if p.startswith('win'):
        platform = 'windows'
    elif p == 'darwin':
        platform = 'macos'
    else:
        platform = 'linux'

target = ARGUMENTS.get('target', 'template_release')
arch = ARGUMENTS.get('arch', 'universal' if platform == 'macos' else 'x86_64')
if arch not in ['x86_64', 'x86_32', 'arm64', 'universal']:
    print(f"ERROR: Invalid architecture '{arch}'. Supported architectures are: x86_64, x86_32, arm64, universal.")
    Exit(1)

# Set up the environment based on the platform
use_mingw_arg = ARGUMENTS.get('use_mingw', 'no')
use_mingw = use_mingw_arg.lower() in ['yes', 'true', '1']

if platform == 'windows' and not use_mingw:
    # Use the MSVC compiler on Windows (native build)
    env = Environment(tools=['default', 'msvc'])
elif platform == 'windows' and use_mingw:
    # Force SCons to use the MinGW toolchain for cross-compilation
    env = Environment(tools=['gcc', 'g++', 'gnulink', 'ar', 'gas'])

    # Explicitly override compiler settings after environment creation
    cc_cmd = os.environ.get('CC', 'x86_64-w64-mingw32-gcc')
    cxx_cmd = os.environ.get('CXX', 'x86_64-w64-mingw32-g++')

    env.Replace(CC=cc_cmd)
    env.Replace(CXX=cxx_cmd)
    env.Replace(LINK=cxx_cmd)  # Use C++ compiler for linking (g++) so libstdc++ and C++ EH symbols are pulled in

else:
    # Use the default compiler on other platforms
    env = Environment()

# Optional: enable SCons cache if SCONS_CACHE_DIR is provided (local or CI)
cache_dir = os.environ.get('SCONS_CACHE_DIR')
if cache_dir:
    CacheDir(cache_dir)

# Add include paths for godot-cpp, yaml-cpp, and libretro
env.Append(CPPPATH=[
    'src',
    '.',
    'godot-cpp/include',
    'godot-cpp/gen/include',
    'godot-cpp/gdextension',
    'libretro',
    'libretro-common/include',
    'yaml-cpp/include',
])

# Platform-specific include paths - only add system paths for native builds
if not use_mingw and platform != 'windows':
    # Only add Linux/macOS system paths for native builds
    env.Append(CPPPATH=['/usr/include', '/usr/local/include'])

if platform == 'windows' and use_mingw:
    # Ensure MinGW uses its own sysroot and headers
    env.Append(CCFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])
    env.Append(LINKFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])

# Platform-specific library paths
env.Append(LIBPATH=['godot-cpp/bin'])

is_windows = platform == 'windows'
if is_windows and not use_mingw:
    # MSVC flags
    env.Append(CCFLAGS=['/std:c++17', '/EHsc'])
    if target == 'template_debug':
        env.Append(CCFLAGS=['/Od', '/MDd', '/Zi'])
        env.Append(LINKFLAGS=['/DEBUG'])
    else:
        env.Append(CCFLAGS=['/O2', '/MD'])
elif is_windows and use_mingw:
    # MinGW flags
    env.Append(CCFLAGS=['-std=c++17'])
    if target == 'template_debug':
        env.Append(CCFLAGS=['-g', '-O0'])
    else:
        env.Append(CCFLAGS=['-O3'])
else:
    # Linux/macOS flags
    env.Append(CCFLAGS=['-std=c++17'])
    if target == 'template_debug':
        env.Append(CCFLAGS=['-g', '-O0'])
    else:
        env.Append(CCFLAGS=['-O3'])

# Determine godot-cpp library name
lib_prefix = "lib"
if is_windows and not use_mingw:
    # MSVC produces .lib files without lib prefix
    lib_prefix = ""
    lib_ext = ".lib"
elif is_windows:
    lib_ext = ".a"
elif platform == 'macos':
    lib_ext = ".a"
else:
    lib_ext = ".a"

# Handle macOS universal builds
if platform == 'macos' and arch == 'universal':
    # For universal builds, godot-cpp creates arch-specific libraries
    # We need to check which one exists
    possible_libs = [
        f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}",
        f"{lib_prefix}godot-cpp.{platform}.{target}.x86_64{lib_ext}",
        f"{lib_prefix}godot-cpp.{platform}.{target}.arm64{lib_ext}",
    ]

    godot_cpp_lib = None
    for lib in possible_libs:
        if os.path.exists(os.path.join('godot-cpp', 'bin', lib)):
            godot_cpp_lib = lib
            break

    if not godot_cpp_lib:
        print(f"ERROR: Could not find godot-cpp library. Tried:")
        for lib in possible_libs:
            print(f"  - {os.path.join('godot-cpp', 'bin', lib)}")
        Exit(1)
else:
    godot_cpp_lib = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"

env.Append(LIBS=[File(os.path.join('godot-cpp', 'bin', godot_cpp_lib))])

# Build godot-cpp only if pre-built library is missing
godot_cpp_lib_path = os.path.join('godot-cpp', 'bin', godot_cpp_lib)
if os.path.exists(godot_cpp_lib_path):
    print(f"Using pre-built godot-cpp: {godot_cpp_lib_path}")
else:
    print("Building godot-cpp (pre-built library not found)...")
    SConscript("godot-cpp/SConstruct")

# Platform-specific system libraries
if is_windows:
    # Windows system libraries
    env.Append(LIBS=['kernel32', 'user32', 'gdi32', 'winspool', 'comdlg32', 'advapi32',
                     'shell32', 'ole32', 'oleaut32', 'uuid', 'odbc32', 'odbccp32'])
elif platform == 'linux':
    env.Append(LIBS=['pthread', 'dl'])
elif platform == 'macos':
    env.Append(LIBS=['pthread'])
    env.Append(FRAMEWORKS=['CoreFoundation'])


# Debug logging: print resolved names and compiler locations
print("=== SCons build configuration ===")
print("Project:", PROJECT_NAME)
print("platform:", platform)
print("target:", target)
print("arch:", arch)
print("use_mingw:", ARGUMENTS.get('use_mingw', 'no'))
print("expected godot_cpp_lib:", godot_cpp_lib)
print("godot-cpp bin path:", os.path.join('godot-cpp', 'bin'))
print("ENV CC:", os.environ.get('CC'))
print("ENV CXX:", os.environ.get('CXX'))
print("env['CC']:", env.get('CC'))
print("env['CXX']:", env.get('CXX'))
print("==================================")

# Source files for LibRetroHost
src_files = [
    'src/RegisterExtension.cpp',
    'src/RetroHost.cpp',
    'src/CoreEnvironment.cpp',
    'src/CoreVariables.cpp',
    'src/Audio.cpp',
    'src/Input.cpp',
    'src/Video.cpp',
    'src/VFS.cpp',
]

# yaml-cpp source files (compiled directly into shared library)
yaml_cpp_files = [
    'yaml-cpp/src/binary.cpp',
    'yaml-cpp/src/convert.cpp',
    'yaml-cpp/src/depthguard.cpp',
    'yaml-cpp/src/directives.cpp',
    'yaml-cpp/src/emit.cpp',
    'yaml-cpp/src/emitfromevents.cpp',
    'yaml-cpp/src/emitter.cpp',
    'yaml-cpp/src/emitterstate.cpp',
    'yaml-cpp/src/emitterutils.cpp',
    'yaml-cpp/src/exceptions.cpp',
    'yaml-cpp/src/exp.cpp',
    'yaml-cpp/src/fptostring.cpp',
    'yaml-cpp/src/memory.cpp',
    'yaml-cpp/src/node.cpp',
    'yaml-cpp/src/node_data.cpp',
    'yaml-cpp/src/nodebuilder.cpp',
    'yaml-cpp/src/nodeevents.cpp',
    'yaml-cpp/src/null.cpp',
    'yaml-cpp/src/ostream_wrapper.cpp',
    'yaml-cpp/src/parse.cpp',
    'yaml-cpp/src/parser.cpp',
    'yaml-cpp/src/regex_yaml.cpp',
    'yaml-cpp/src/scanner.cpp',
    'yaml-cpp/src/scanscalar.cpp',
    'yaml-cpp/src/scantag.cpp',
    'yaml-cpp/src/scantoken.cpp',
    'yaml-cpp/src/simplekey.cpp',
    'yaml-cpp/src/singledocparser.cpp',
    'yaml-cpp/src/stream.cpp',
    'yaml-cpp/src/tag.cpp',
    'yaml-cpp/src/contrib/graphbuilder.cpp',
    'yaml-cpp/src/contrib/graphbuilderadapter.cpp',
]

src_files += yaml_cpp_files

# Determine output directory based on platform
if platform == 'windows':
    output_dir = f'demo/bin/{PROJECT_NAME}/lib/Windows-AMD64'
elif platform == 'linux':
    output_dir = f'demo/bin/{PROJECT_NAME}/lib/Linux-x86_64'
elif platform == 'macos':
    output_dir = f'demo/bin/{PROJECT_NAME}/lib/Darwin-Universal'
else:
    output_dir = f'demo/bin/{PROJECT_NAME}/lib/{platform}'

# Create output directory
env.Execute(Mkdir(output_dir))

# Set the correct library suffix and prefix based on platform
if is_windows:
    env['SHLIBPREFIX'] = ''  # No prefix for Windows to match CMake output
    env['SHLIBSUFFIX'] = '-d.dll' if target == 'template_debug' else '.dll'
elif platform == 'macos':
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.dylib'
else:
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.so'

# Debug logging: shared lib name details
print("SHLIBPREFIX:", env.get('SHLIBPREFIX'))
print("SHLIBSUFFIX:", env.get('SHLIBSUFFIX'))
print("Output directory:", output_dir)
print("Target shared lib will be created as:", env.get('SHLIBPREFIX') + PROJECT_NAME + env.get('SHLIBSUFFIX'))

# Build the LibRetroHost shared library
library_target = os.path.join(output_dir, PROJECT_NAME)
library = env.SharedLibrary(target=library_target, source=src_files)
Default(library)

print(f"Build target: {library_target}")
print(f"Build complete!")
