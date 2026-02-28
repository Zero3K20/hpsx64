/*
 * linux_main.cpp - Linux entry point that calls WinMain
 * This provides main() for Linux builds of hpsx64.
 */

#include "windows_compat.h"
#include <SDL2/SDL.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

int main(int argc, char* argv[])
{
    // Change the working directory to the folder that contains the binary so
    // that config files (*.hcfg) and save files (card*) are always found
    // relative to the executable, regardless of how the application was
    // launched (file manager, Steam, terminal from a different directory, …).
    char exe_path[PATH_MAX] = {};
    ssize_t exe_len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (exe_len > 0) {
        exe_path[exe_len] = '\0';
        char* last_slash = strrchr(exe_path, '/');
        // Skip if the binary is directly in the root directory (edge case; avoid
        // calling chdir("/") which would hide the real binary directory).
        if (last_slash && last_slash != exe_path) {
            *last_slash = '\0';
            chdir(exe_path);
        }
    }

    // On Steam Deck (Gamescope) and other Wayland compositors, prefer X11/XWayland
    // for SDL2 video so that OpenGL context creation succeeds reliably.
    // Only set SDL_VIDEODRIVER when the caller has not already done so.
    bool we_set_videodriver = false;
    if (!getenv("SDL_VIDEODRIVER")) {
        setenv("SDL_VIDEODRIVER", "x11", 0);
        we_set_videodriver = true;
    }

    // Initialize SDL2 subsystems.  If the x11 driver fails (e.g. DISPLAY is
    // not set in a pure-Wayland environment), fall back to SDL's auto-
    // detection so that the Wayland backend can be tried instead.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        if (we_set_videodriver) {
            unsetenv("SDL_VIDEODRIVER");
            if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
                fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
                return 1;
            }
        } else {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            return 1;
        }
    }

    // Call the WinMain equivalent
    int result = WinMain(nullptr, nullptr, (argc > 1) ? argv[1] : (char*)"", 1);

    SDL_Quit();
    return result;
}
