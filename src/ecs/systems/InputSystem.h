#pragma once
#include <SDL3/SDL.h>

struct InputState {
    int mouseX = 0;
    int mouseY = 0;
    bool quit = false;

    // Key press events (single frame)
    bool upPressed = false;
    bool downPressed = false;
    bool enterPressed = false;
    bool escapePressed = false;
};

class InputSystem {
public:
    void setWindow(SDL_Window* window) {
        m_window = window;
    }

    InputState pollEvents() {
        InputState state;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                state.quit = true;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_ESCAPE:
                        state.escapePressed = true;
                        break;
                    case SDLK_UP:
                        state.upPressed = true;
                        break;
                    case SDLK_DOWN:
                        state.downPressed = true;
                        break;
                    case SDLK_RETURN:
                        state.enterPressed = true;
                        break;
                }
            }
            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                state.mouseX += static_cast<int>(event.motion.xrel);
                state.mouseY += static_cast<int>(event.motion.yrel);
            }
        }

        return state;
    }

    void captureMouse(bool capture) {
        if (m_window) {
            SDL_SetWindowRelativeMouseMode(m_window, capture);
        }
    }

private:
    SDL_Window* m_window = nullptr;
};
