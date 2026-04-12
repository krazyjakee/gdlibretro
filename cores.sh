#!/bin/bash

set -e

PLATFORM="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CORES_DIR="$SCRIPT_DIR/demo/libretro-cores"
ARCHIVE_NAME="RetroArch_cores.7z"

if [ -z "$PLATFORM" ]; then
    echo "Usage: $0 <platform>"
    echo "Platforms: linux, windows, macos"
    exit 1
fi

# Check for curl or wget
if ! command -v curl &> /dev/null && ! command -v wget &> /dev/null; then
    echo "curl or wget is required but neither is installed."
    exit 1
fi

download() {
    local url="$1"
    local output="$2"
    if command -v curl &> /dev/null; then
        curl -L -f -o "$output" "$url"
    else
        wget -O "$output" "$url"
    fi
}

# Create cores directory if it doesn't exist
mkdir -p "$CORES_DIR"

if [ "$PLATFORM" = "macos" ]; then
    # macOS cores are distributed as individual .dylib.zip files per architecture.
    # Download both arm64 and x86_64, then lipo them into universal binaries.
    BASE_URL="https://buildbot.libretro.com/nightly/apple/osx"
    ARM64_DIR="$CORES_DIR/tmp_arm64"
    X86_64_DIR="$CORES_DIR/tmp_x86_64"
    mkdir -p "$ARM64_DIR" "$X86_64_DIR"

    echo "Fetching core list..."
    INDEX_HTML=$(curl -s -f "$BASE_URL/arm64/latest/")
    CORE_ZIPS=$(echo "$INDEX_HTML" | grep -oE '[a-zA-Z0-9_]+_libretro\.dylib\.zip' | sort -u)

    TOTAL=$(echo "$CORE_ZIPS" | wc -l | tr -d ' ')
    echo "Found $TOTAL cores. Downloading arm64 and x86_64 builds..."

    COUNT=0
    for zip in $CORE_ZIPS; do
        COUNT=$((COUNT + 1))
        CORE_NAME="${zip%.zip}"
        echo "[$COUNT/$TOTAL] $CORE_NAME"

        # Download and extract both architectures
        download "$BASE_URL/arm64/latest/$zip" "$ARM64_DIR/$zip" 2>/dev/null || { echo "  (arm64 not available, skipping)"; continue; }
        download "$BASE_URL/x86_64/latest/$zip" "$X86_64_DIR/$zip" 2>/dev/null || { echo "  (x86_64 not available, arm64 only)"; unzip -o -q "$ARM64_DIR/$zip" -d "$CORES_DIR"; continue; }

        unzip -o -q "$ARM64_DIR/$zip" -d "$ARM64_DIR"
        unzip -o -q "$X86_64_DIR/$zip" -d "$X86_64_DIR"

        # Create universal binary with lipo
        if [ -f "$ARM64_DIR/$CORE_NAME" ] && [ -f "$X86_64_DIR/$CORE_NAME" ]; then
            ARM64_ARCHS=$(lipo -archs "$ARM64_DIR/$CORE_NAME" 2>/dev/null || true)
            X86_64_ARCHS=$(lipo -archs "$X86_64_DIR/$CORE_NAME" 2>/dev/null || true)
            if [ "$ARM64_ARCHS" = "$X86_64_ARCHS" ]; then
                echo "  (both builds are $ARM64_ARCHS, skipping lipo)"
                cp "$ARM64_DIR/$CORE_NAME" "$CORES_DIR/$CORE_NAME"
            else
                lipo -create "$ARM64_DIR/$CORE_NAME" "$X86_64_DIR/$CORE_NAME" -output "$CORES_DIR/$CORE_NAME"
            fi
        elif [ -f "$ARM64_DIR/$CORE_NAME" ]; then
            cp "$ARM64_DIR/$CORE_NAME" "$CORES_DIR/$CORE_NAME"
        fi

        # Clean up extracted dylibs for next iteration
        rm -f "$ARM64_DIR/$CORE_NAME" "$X86_64_DIR/$CORE_NAME"
    done

    echo "Cleaning up..."
    rm -rf "$ARM64_DIR" "$X86_64_DIR"

    echo "Done! Universal cores extracted to $CORES_DIR"
    exit 0
fi

# Linux/Windows: single .7z archive
case "$PLATFORM" in
    linux)
        URL="https://buildbot.libretro.com/nightly/linux/x86_64/$ARCHIVE_NAME"
        ;;
    windows)
        URL="https://buildbot.libretro.com/nightly/windows/x86_64/$ARCHIVE_NAME"
        ;;
    *)
        echo "Unknown platform: $PLATFORM"
        echo "Supported platforms: linux, windows, macos"
        exit 1
        ;;
esac

# Check for 7z (only needed for linux/windows)
if ! command -v 7z &> /dev/null; then
    echo "7z is required but not installed."
    echo "Install with: sudo apt install p7zip-full (Debian/Ubuntu)"
    exit 1
fi

echo "Downloading $PLATFORM cores..."
cd "$CORES_DIR"

download "$URL" "$ARCHIVE_NAME"

echo "Extracting cores..."
7z x -y "$ARCHIVE_NAME"

echo "Fixing folder structure..."
case "$PLATFORM" in
    linux)
        mv RetroArch-Linux-x86_64/RetroArch-Linux-x86_64.AppImage.home/.config/retroarch/cores/* .
        rm -rf RetroArch-Linux-x86_64
        ;;
    windows)
        mv RetroArch-Win64/cores/* .
        rm -rf RetroArch-Win64
        ;;
esac

echo "Cleaning up..."
rm -f "$ARCHIVE_NAME"

echo "Done! Cores extracted to $CORES_DIR"
