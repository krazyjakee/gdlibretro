#!/bin/bash

set -e

PLATFORM="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CORES_DIR="$SCRIPT_DIR/demo/libretro-cores"
ARCHIVE_NAME="RetroArch_cores.7z"

if [ -z "$PLATFORM" ]; then
    echo "Usage: $0 <platform>"
    echo "Platforms: linux, windows"
    exit 1
fi

case "$PLATFORM" in
    linux)
        URL="https://buildbot.libretro.com/nightly/linux/x86_64/$ARCHIVE_NAME"
        ;;
    windows)
        URL="https://buildbot.libretro.com/nightly/windows/x86_64/$ARCHIVE_NAME"
        ;;
    *)
        echo "Unknown platform: $PLATFORM"
        echo "Supported platforms: linux, windows"
        exit 1
        ;;
esac

# Check for required tools
if ! command -v 7z &> /dev/null; then
    echo "7z is required but not installed."
    echo "Install with: sudo apt install p7zip-full (Debian/Ubuntu)"
    exit 1
fi

if ! command -v curl &> /dev/null && ! command -v wget &> /dev/null; then
    echo "curl or wget is required but neither is installed."
    exit 1
fi

# Create cores directory if it doesn't exist
mkdir -p "$CORES_DIR"

echo "Downloading $PLATFORM cores..."
cd "$CORES_DIR"

# Download using curl or wget
if command -v curl &> /dev/null; then
    curl -L -o "$ARCHIVE_NAME" "$URL"
else
    wget -O "$ARCHIVE_NAME" "$URL"
fi

echo "Extracting cores..."
7z x -y "$ARCHIVE_NAME"

echo "Fixing folder structure..."
# Move cores from nested structure to top level
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
