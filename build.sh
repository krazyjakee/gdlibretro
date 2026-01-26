#!/bin/bash

# Build script for GDLibretro (Libretro GDExtension for Godot)
# This script builds the LibRetroHost extension using SCons

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# --- Configuration ---
PLATFORM=""
ARCH="x86_64"
SCONS_FLAGS=""
PROJECT_NAME="LibRetroHost"

# --- Helper Functions ---
show_usage() {
    echo -e "${YELLOW}Usage: $0 [linux|macos|windows] [options]${NC}"
    echo "  linux: Build for Linux (x86_64)"
    echo "  macos: Build for macOS (universal)"
    echo "  windows: Build for Windows (x86_64, cross-compile)"
    echo ""
    echo "Options:"
    echo "  --debug: Build debug version (default: release)"
    exit 1
}

# --- Platform-specific Setup ---
setup_linux() {
    PLATFORM="linux"
    ARCH="x86_64"
    SCONS_FLAGS="platform=linux"
    echo -e "${BLUE}=== GDLibretro Build Script (Linux) ===${NC}"
}

setup_macos() {
    PLATFORM="macos"
    ARCH="universal"
    SCONS_FLAGS="platform=macos arch=universal"
    echo -e "${BLUE}=== GDLibretro Build Script (macOS) ===${NC}"
}

setup_windows() {
    PLATFORM="windows"
    ARCH="x86_64"
    SCONS_FLAGS="platform=windows use_mingw=yes"
    echo -e "${BLUE}=== GDLibretro Build Script (Windows Cross-Compile) ===${NC}"
}

# --- Build Functions ---

# Function to check if godot-cpp cache is valid
check_godotcpp_cache() {
    echo -e "${YELLOW}Checking godot-cpp cache...${NC}"

    # When cross-compiling with MinGW, we still get .a files, not .lib files
    # .lib files are only produced when building with MSVC on Windows
    local lib_ext="a"

    local required_files=(
        "godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_release.${ARCH}.${lib_ext}"
        "godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_debug.${ARCH}.${lib_ext}"
        "godot-cpp/gen/include"
        "godot-cpp/gen/src"
    )

    for file in "${required_files[@]}"; do
        if [ ! -e "$file" ]; then
            echo -e "${RED}Cache miss: $file not found${NC}"
            return 1
        fi
    done

    if [ -f "godot-cpp/.sconsign.dblite" ]; then
        echo -e "${GREEN}SCons signature file found${NC}"
    fi

    echo -e "${GREEN}godot-cpp cache is valid!${NC}"
    return 0
}

# Function to build godot-cpp
build_godotcpp() {
    echo -e "${YELLOW}Building godot-cpp...${NC}"

    cd godot-cpp

    echo -e "${BLUE}Building template_release...${NC}"
    scons $SCONS_FLAGS generate_bindings=yes target=template_release

    echo -e "${BLUE}Building template_debug...${NC}"
    scons $SCONS_FLAGS generate_bindings=yes target=template_debug

    cd ..

    echo -e "${GREEN}godot-cpp build completed!${NC}"
}

# Function to install dependencies
install_dependencies() {
    echo -e "${YELLOW}Checking dependencies for ${PLATFORM}...${NC}"

    local required_tools=("scons")

    if [ "$PLATFORM" == "linux" ]; then
        if [[ "$OSTYPE" != "linux-gnu"* ]]; then
            echo -e "${RED}Linux build requires a Linux environment. Current OS: $OSTYPE${NC}"
            exit 1
        fi
        required_tools+=("g++")
    elif [ "$PLATFORM" == "windows" ]; then
        required_tools+=("x86_64-w64-mingw32-gcc" "x86_64-w64-mingw32-g++")
    elif [ "$PLATFORM" == "macos" ]; then
        if [[ "$OSTYPE" != "darwin"* ]]; then
            echo -e "${RED}macOS build requires a macOS environment. Current OS: $OSTYPE${NC}"
            exit 1
        fi
        required_tools+=("clang++")
    fi

    local missing_tools=()
    for tool in "${required_tools[@]}"; do
        if ! command -v "$tool" &> /dev/null; then
            missing_tools+=("$tool")
        fi
    done

    if [ ${#missing_tools[@]} -ne 0 ]; then
        echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
        echo -e "${YELLOW}Please install them for your system.${NC}"
        exit 1
    fi

    echo -e "${GREEN}All required dependencies are available${NC}"
}

# Function to build the main project
build_main_project() {
    echo -e "${YELLOW}Building ${PROJECT_NAME}...${NC}"
    scons $SCONS_FLAGS
    echo -e "${GREEN}${PROJECT_NAME} build completed!${NC}"
}

# Function to show cache status
show_cache_status() {
    echo -e "${BLUE}=== Build Cache Status ===${NC}"

    # When cross-compiling with MinGW, we still get .a files, not .lib files
    # .lib files are only produced when building with MSVC on Windows
    local lib_ext="a"

    if [ -f "godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_release.${ARCH}.${lib_ext}" ]; then
        echo -e "${GREEN}✓ Release library cached${NC}"
    else
        echo -e "${RED}✗ Release library missing${NC}"
    fi

    if [ -f "godot-cpp/bin/libgodot-cpp.${PLATFORM}.template_debug.${ARCH}.${lib_ext}" ]; then
        echo -e "${GREEN}✓ Debug library cached${NC}"
    else
        echo -e "${RED}✗ Debug library missing${NC}"
    fi

    if [ -d "godot-cpp/gen/include" ] && [ -d "godot-cpp/gen/src" ]; then
        echo -e "${GREEN}✓ Generated bindings cached${NC}"
    else
        echo -e "${RED}✗ Generated bindings missing${NC}"
    fi

    if [ -f "godot-cpp/.sconsign.dblite" ]; then
        echo -e "${GREEN}✓ SCons signature file present${NC}"
    else
        echo -e "${YELLOW}! SCons signature file missing (will be created)${NC}"
    fi

    echo ""
}

# --- Main Execution ---
main() {
    # Parse command-line arguments
    if [ -z "$1" ]; then
        show_usage
    fi

    local BUILD_TARGET="template_release"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            linux)
                setup_linux
                shift
                ;;
            macos)
                setup_macos
                shift
                ;;
            windows)
                export CC=x86_64-w64-mingw32-gcc
                export CXX=x86_64-w64-mingw32-g++
                setup_windows
                shift
                ;;
            --debug)
                BUILD_TARGET="template_debug"
                shift
                ;;
            *)
                show_usage
                ;;
        esac
    done

    if [ -z "$PLATFORM" ]; then
        show_usage
    fi

    # Append target to SCONS_FLAGS
    SCONS_FLAGS="$SCONS_FLAGS target=$BUILD_TARGET"

    echo -e "${BLUE}Platform: ${PLATFORM}, Architecture: ${ARCH}, Target: ${BUILD_TARGET}${NC}"
    echo ""

    install_dependencies
    show_cache_status

    if ! check_godotcpp_cache; then
        build_godotcpp
    else
        echo -e "${GREEN}Using cached godot-cpp build${NC}"
    fi

    echo ""
    build_main_project
    echo ""

    echo -e "${GREEN}=== Build Complete! ===${NC}"
    echo -e "${BLUE}Built for: ${PLATFORM} (${ARCH})${NC}"
    echo -e "${BLUE}Build type: ${BUILD_TARGET}${NC}"

    if [ "$PLATFORM" == "windows" ]; then
        echo -e "${BLUE}Output: demo/bin/${PROJECT_NAME}/lib/Windows-AMD64/${PROJECT_NAME}*.dll${NC}"
    elif [ "$PLATFORM" == "linux" ]; then
        echo -e "${BLUE}Output: demo/bin/${PROJECT_NAME}/lib/Linux-x86_64/lib${PROJECT_NAME}.so${NC}"
    elif [ "$PLATFORM" == "macos" ]; then
        echo -e "${BLUE}Output: demo/bin/${PROJECT_NAME}/lib/Darwin-Universal/lib${PROJECT_NAME}.dylib${NC}"
    fi

    show_cache_status
}

main "$@"
