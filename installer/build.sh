#!/bin/bash
# build.sh - Build script for HarukaInstaller
# Usage:
#   ./build.sh           - Build the installer
#   ./build.sh test-ui   - Build the TUI test (with simulated data)
#   ./build.sh clean     - Clean build artifacts

set -e

CXX="${CXX:-g++}"
CXXFLAGS="-std=c++17 -Wall -Wextra -I./lib/ncurses"
LDFLAGS="-lncursesw"

SRCDIR="./lib/ncurses"
OUTDIR="./build"

mkdir -p "$OUTDIR"

case "${1:-}" in
    test-ui)
        echo "==> Building TUI test..."
        $CXX $CXXFLAGS -DTESTUI \
            "$SRCDIR/ncurseslib.cpp" \
            "$SRCDIR/mainmenu/mainmenu.cpp" \
            "$SRCDIR/test/testing.cpp" \
            $LDFLAGS \
            -o "$OUTDIR/test_ui"
        echo "==> Built: $OUTDIR/test_ui"
        echo "    Run with: $OUTDIR/test_ui"
        ;;

    clean)
        echo "==> Cleaning build artifacts..."
        rm -rf "$OUTDIR"
        rm -rf cmake-build
        echo "==> Done."
        ;;

    cmake)
        echo "==> Building with CMake..."
        mkdir -p cmake-build && cd cmake-build
        cmake -DBUILD_TEST_UI=ON ..
        cmake --build .
        echo "==> Built: cmake-build/test_ui"
        ;;

    *)
        echo "==> Building HarukaInstaller..."
        echo "    (full build not yet implemented, use 'test-ui' for now)"
        echo ""
        echo "Usage: ./build.sh [test-ui|cmake|clean]"
        ;;
esac
