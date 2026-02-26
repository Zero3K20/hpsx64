#!/bin/bash
# MakefilePS2_Linux.sh - Build script for hps2x64 (PS2 emulator) on Linux/Steam Deck
# Usage: ./MakefilePS2_Linux.sh [clean]

set -e

# Check for required tools
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "ERROR: $1 not found. Install it first."
        echo "  Steam Deck/Arch: sudo pacman -S $2"
        echo "  Ubuntu/Debian:   sudo apt-get install $3"
        exit 1
    fi
}

check_dep g++ gcc g++
check_dep make make make

# Check for SDL2
if ! pkg-config --exists sdl2 2>/dev/null; then
    echo "WARNING: SDL2 not found via pkg-config. Build may fail."
    echo "  Steam Deck/Arch: sudo pacman -S sdl2"
    echo "  Ubuntu/Debian:   sudo apt-get install libsdl2-dev"
fi

# Check for OpenGL
if ! pkg-config --exists gl 2>/dev/null; then
    echo "WARNING: OpenGL not found via pkg-config."
    echo "  Steam Deck/Arch: sudo pacman -S mesa"
    echo "  Ubuntu/Debian:   sudo apt-get install libgl1-mesa-dev"
fi

make -f MakefilePS2_Linux.txt "$@"
echo ""
echo "Build complete: hps2x64"
