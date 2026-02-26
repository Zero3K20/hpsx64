/*
 * linux_main.cpp - Linux entry point that calls WinMain
 * This provides main() for Linux builds of hpsx64.
 */

#include "windows_compat.h"
#include <SDL2/SDL.h>

int main(int argc, char* argv[])
{
    // Initialize SDL2 subsystems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Call the WinMain equivalent
    int result = WinMain(nullptr, nullptr, (argc > 1) ? argv[1] : (char*)"", 1);

    SDL_Quit();
    return result;
}
