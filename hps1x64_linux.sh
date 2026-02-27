#!/bin/bash
# hps1x64_linux.sh - Launcher script for hps1x64 on Linux / Steam Deck
#
# Steam Deck usage:
#   1. Copy hps1x64_linux and hps1x64_linux.sh to the same folder.
#   2. In Steam (Desktop Mode) choose "Add a Non-Steam Game", browse to this
#      script, and add it.
#   3. Launch from Steam or Gaming Mode.
#
# The script ensures:
#   - The working directory is the folder containing the binary (so that config
#     files such as hps1x64.hcfg and card* are found correctly).
#   - The binary has execute permission.
#   - SDL2 uses X11/XWayland on Gamescope (Steam Deck Gaming Mode) when no
#     SDL_VIDEODRIVER is already set in the environment.

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
cd "$SCRIPT_DIR" || exit 1

chmod +x ./hps1x64_linux 2>/dev/null

export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"

exec ./hps1x64_linux "$@"
