# Variables
SCRIPT_DIR := $(shell cd $(dirname $(realpath $(lastword $(MAKEFILE_LIST)))) && pwd)
ADDONS_DIR := $(SCRIPT_DIR)/addons
BUILD_DIR := build

# Rules
.PHONY: all submodules build clean

# Default rule (will be executed when running `make`)
all: submodules build move_files clean

# Update Git submodules
submodules:
	git submodule update --init --recursive

# Generate build files and compile the project

JOBS := 5
build:
	cmake -G Ninja -DNO_GIT_REVISION=ON -DCMAKE_BUILD_TYPE=Release -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR) -- -j ${JOBS}

# Move the generated files to the addons directory
move_files:
	find . -name "gdlibretro-d.*" -exec mv -v {} $(ADDONS_DIR)/ \;

# Clean the build files
clean:
	rm -rf $(BUILD_DIR)

ARCH := $(shell uname -m)
CORE_NAME := dosbox_pure
get-core/apple:
	cd libretro-cores && \
	curl -L https://buildbot.libretro.com/nightly/apple/osx/${ARCH}/latest/${CORE_NAME}_libretro.dylib.zip | bsdtar -xvf -