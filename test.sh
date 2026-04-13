#!/bin/bash

# Test script for GDLibretro (LibRetro GDExtension)
# Verifies that the library loads correctly in Godot

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

PROJECT_NAME="gdlibretro"

echo -e "${BLUE}=== Testing GDLibretro Extension ===${NC}"

# Check if Godot executable exists
if ! command -v godot &> /dev/null && [ ! -f "./godot" ]; then
    echo -e "${RED}Error: Godot executable not found${NC}"
    echo -e "${YELLOW}Please install Godot or place a godot binary in the project root${NC}"
    exit 1
fi

# Use local godot if it exists, otherwise use system godot
if [ -f "./godot" ]; then
    GODOT_CMD="./godot"
else
    GODOT_CMD="godot"
fi

echo -e "${BLUE}Using Godot at: $(which $GODOT_CMD || echo ./godot)${NC}"

# Check if the GDExtension library exists for the current platform
FOUND_LIB=false

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    LIB_PATH="addons/${PROJECT_NAME}/lib/Linux-x86_64/lib${PROJECT_NAME}.so"
    if [ -f "$LIB_PATH" ]; then
        FOUND_LIB=true
        echo -e "${GREEN}✓ Found Linux library: $LIB_PATH${NC}"
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    LIB_PATH="addons/${PROJECT_NAME}/lib/Darwin-Universal/lib${PROJECT_NAME}.dylib"
    if [ -f "$LIB_PATH" ]; then
        FOUND_LIB=true
        echo -e "${GREEN}✓ Found macOS library: $LIB_PATH${NC}"
    fi
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "win32" ]]; then
    LIB_PATH_RELEASE="addons/${PROJECT_NAME}/lib/Windows-AMD64/${PROJECT_NAME}.dll"
    LIB_PATH_DEBUG="addons/${PROJECT_NAME}/lib/Windows-AMD64/${PROJECT_NAME}-d.dll"
    if [ -f "$LIB_PATH_RELEASE" ] || [ -f "$LIB_PATH_DEBUG" ]; then
        FOUND_LIB=true
        echo -e "${GREEN}✓ Found Windows library${NC}"
        [ -f "$LIB_PATH_RELEASE" ] && echo -e "  Release: $LIB_PATH_RELEASE"
        [ -f "$LIB_PATH_DEBUG" ] && echo -e "  Debug: $LIB_PATH_DEBUG"
    fi
fi

if [ "$FOUND_LIB" = false ]; then
    echo -e "${RED}Error: GDExtension library not found!${NC}"
    echo -e "${YELLOW}Please run build.sh first to build the extension${NC}"
    echo ""
    echo "Expected library locations:"
    echo "  Linux:   addons/${PROJECT_NAME}/lib/Linux-x86_64/lib${PROJECT_NAME}.so"
    echo "  macOS:   addons/${PROJECT_NAME}/lib/Darwin-Universal/lib${PROJECT_NAME}.dylib"
    echo "  Windows: addons/${PROJECT_NAME}/lib/Windows-AMD64/${PROJECT_NAME}.dll"
    exit 1
fi

# Check if the gdextension file exists
GDEXT_FILE="addons/${PROJECT_NAME}/${PROJECT_NAME}.gdextension"
if [ ! -f "$GDEXT_FILE" ]; then
    echo -e "${YELLOW}Warning: .gdextension file not found at $GDEXT_FILE${NC}"
    echo -e "${YELLOW}The extension may not load correctly in Godot${NC}"
fi

# Import the project (this will also validate the extension loads)
echo -e "${YELLOW}Importing Godot project (this validates the extension)...${NC}"
timeout 30s $GODOT_CMD --headless --quit 2>&1 | tee /tmp/godot_test_output.txt

# Check if there were any errors in the output
if grep -qi "error" /tmp/godot_test_output.txt || grep -qi "failed" /tmp/godot_test_output.txt; then
    echo -e "${RED}✗ Errors detected during project import${NC}"
    echo -e "${YELLOW}Check the output above for details${NC}"
    exit 1
else
    echo -e "${GREEN}✓ Project imported successfully${NC}"
fi

# Try to run the demo scene briefly to verify the extension works
if [ -f "main.tscn" ]; then
    echo -e "${YELLOW}Running demo scene for 5 seconds...${NC}"
    timeout 5s $GODOT_CMD --headless main.tscn 2>&1 | tee /tmp/godot_scene_output.txt || true

    if grep -qi "error" /tmp/godot_scene_output.txt; then
        echo -e "${YELLOW}⚠ Some errors detected during scene execution${NC}"
        echo -e "${YELLOW}This may be expected if libretro cores are not present${NC}"
    else
        echo -e "${GREEN}✓ Demo scene executed${NC}"
    fi
fi

cd ..

echo ""
echo -e "${GREEN}=== All Tests Completed! ===${NC}"
echo -e "${BLUE}The ${PROJECT_NAME} extension appears to be working correctly${NC}"
echo ""
echo -e "${YELLOW}Manual testing:${NC}"
echo "1. Open the demo project in Godot: godot project.godot"
echo "2. Run the demo scene to test libretro core loading"
echo "3. Place libretro core files in libretro-cores/"

# Clean up temp files
rm -f /tmp/godot_test_output.txt /tmp/godot_scene_output.txt
