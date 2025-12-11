#pragma once
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include "GameConfig.h"

class WindowManager {
public:
    WindowManager() = default;
    ~WindowManager() { shutdown(); }

    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

    bool init() {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        if (!TTF_Init()) {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return false;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        m_window = SDL_CreateWindow(
            GameConfig::WINDOW_TITLE.c_str(),
            GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
            SDL_WINDOW_OPENGL
        );
        if (!m_window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        m_glContext = SDL_GL_CreateContext(m_window);
        if (!m_glContext) {
            std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            std::cerr << "Failed to initialize GLAD" << std::endl;
            SDL_GL_DestroyContext(m_glContext);
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
            m_glContext = nullptr;
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

        glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        glEnable(GL_DEPTH_TEST);

        m_initialized = true;
        return true;
    }

    void shutdown() {
        if (!m_initialized) return;

        if (m_glContext) {
            SDL_GL_DestroyContext(m_glContext);
            m_glContext = nullptr;
        }
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
        TTF_Quit();
        SDL_Quit();

        m_initialized = false;
    }

    void swapBuffers() {
        if (m_window) SDL_GL_SwapWindow(m_window);
    }

    SDL_Window* window() const { return m_window; }
    SDL_GLContext glContext() const { return m_glContext; }
    bool isInitialized() const { return m_initialized; }
    static int width() { return GameConfig::WINDOW_WIDTH; }
    static int height() { return GameConfig::WINDOW_HEIGHT; }
    static float aspectRatio() { return static_cast<float>(GameConfig::WINDOW_WIDTH) / GameConfig::WINDOW_HEIGHT; }

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    bool m_initialized = false;
};
