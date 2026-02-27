#!/bin/bash
# hps2x64_linux.sh - Launcher script for hps2x64 on Linux / Steam Deck
#
# Steam Deck usage:
#   1. Copy hps2x64_linux and hps2x64_linux.sh to the same folder.
#   2. In Steam (Desktop Mode) choose "Add a Non-Steam Game", browse to this
#      script, and add it.
#   3. Launch from Steam or Gaming Mode.
#
# The script ensures:
#   - The working directory is the folder containing the binary (so that config
#     files such as hps2x64.hcfg and ps2card* are found correctly).
#   - The binary has execute permission.
#   - SDL2 uses X11/XWayland on Gamescope (Steam Deck Gaming Mode) when no
#     SDL_VIDEODRIVER is already set in the environment.

# Resolve the real path of this script, then cd into that directory so that
# relative config/save paths used by hps2x64 work regardless of where Steam
# launched this script from.
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
cd "$SCRIPT_DIR" || exit 1

# Ensure the binary is executable (GitHub release downloads do not preserve
# Unix file permissions, so users may need this on first run).
chmod +x ./hps2x64_linux 2>/dev/null

# On Steam Deck Gaming Mode the display is managed by Gamescope.  SDL2 works
# best with X11/XWayland there.  Only set SDL_VIDEODRIVER when the environment
# has not already overridden it (e.g. to "wayland").
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"

exec ./hps2x64_linux "$@"
