#pragma once
#include <SDL.h>

struct InputState {
    int mouseX = 0;
    int mouseY = 0;
    bool quit = false;
};

class InputSystem {
public:
    InputState pollEvents() {
        InputState state;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                state.quit = true;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                state.quit = true;
            }
            if (event.type == SDL_MOUSEMOTION) {
                state.mouseX += event.motion.xrel;
                state.mouseY += event.motion.yrel;
            }
        }

        return state;
    }

    void captureMouse(bool capture) {
        SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
    }
};
