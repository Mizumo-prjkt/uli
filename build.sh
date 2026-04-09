#!/bin/bash
set -e

BUILD_DIR="build"
DEBUG_ON=0
BUILD_PATCHER=OFF
BUILD_INSTALLER=OFF
BUILD_COMPOSITOR=OFF
SPECIFIC_TARGETS=0

# Usage info
show_help() {
    echo "ULI Build Script"
    echo "Usage: ./build.sh [options]"
    echo ""
    echo "Options:"
    echo "  -d, --debug       Enable debug mode"
    echo "  -p, --patcher     Build only the ISO patcher"
    echo "  -i, --installer   Build only the core installer"
    echo "  -c, --compositor  Build only the GUI compositor"
    echo "  -a, --all         Build all components (default)"
    echo "  -h, --help        Show this help"
}

# Parse arguments
for arg in "$@"; do
    case $arg in
        --debug|-d)
            DEBUG_ON=1
            shift
            ;;
        --patcher|-p)
            BUILD_PATCHER=ON
            SPECIFIC_TARGETS=1
            shift
            ;;
        --installer|-i)
            BUILD_INSTALLER=ON
            SPECIFIC_TARGETS=1
            shift
            ;;
        --compositor|-c)
            BUILD_COMPOSITOR=ON
            SPECIFIC_TARGETS=1
            shift
            ;;
        --all|-a)
            BUILD_PATCHER=ON
            BUILD_INSTALLER=ON
            BUILD_COMPOSITOR=ON
            SPECIFIC_TARGETS=0
            shift
            ;;
        --help|-h)
            show_help
            exit 0
            ;;
    esac
done

# Default to all if no specific targets requested
if [ $SPECIFIC_TARGETS -eq 0 ]; then
    BUILD_PATCHER=ON
    BUILD_INSTALLER=ON
    BUILD_COMPOSITOR=ON
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

CMAKE_ARGS="-DULI_BUILD_PATCHER=$BUILD_PATCHER -DULI_BUILD_INSTALLER=$BUILD_INSTALLER -DULI_BUILD_COMPOSITOR=$BUILD_COMPOSITOR"

if [ $DEBUG_ON -eq 1 ]; then
    echo "Configuring with Debug Mode..."
    CMAKE_ARGS="$CMAKE_ARGS -DULI_DEBUG_MODE=ON"
else
    echo "Configuring for Release..."
    CMAKE_ARGS="$CMAKE_ARGS -DULI_DEBUG_MODE=OFF"
fi

cmake $CMAKE_ARGS ..

# Build
if [ $SPECIFIC_TARGETS -eq 1 ]; then
    TARGETS=""
    [ "$BUILD_PATCHER" == "ON" ] && TARGETS="$TARGETS uli_patcher"
    [ "$BUILD_INSTALLER" == "ON" ] && TARGETS="$TARGETS uli_installer"
    [ "$BUILD_COMPOSITOR" == "ON" ] && TARGETS="$TARGETS uli_compositor"
    echo "Building targets:$TARGETS"
    make -j$(nproc) $TARGETS
else
    echo "Building all components..."
    make -j$(nproc)
fi

echo "------------------------------------------------"
echo "Build complete."
[ "$BUILD_INSTALLER" == "ON" ] && echo "Installer: $(pwd)/uli_installer"
[ "$BUILD_PATCHER" == "ON" ] && echo "Patcher:   $(pwd)/uli_patcher"
[ "$BUILD_COMPOSITOR" == "ON" ] && echo "Compositor: $(pwd)/compositor/uli_compositor"
