#pragma once
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include "GameConfig.h"

// Encapsulates SDL window and OpenGL context creation/destruction
// Handles all low-level initialization so main.cpp stays clean

class WindowManager {
public:
    WindowManager() = default;
    ~WindowManager() { shutdown(); }

    // Non-copyable
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

    // Initialize SDL, TTF, create window and GL context
    // Returns true on success, false on failure
    bool init() {
        // SDL3 init
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        // SDL_ttf init
        if (!TTF_Init()) {
            std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return false;
        }

        // OpenGL attributes
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        // Create window
        m_window = SDL_CreateWindow(
            GameConfig::WINDOW_TITLE,
            GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
            SDL_WINDOW_OPENGL
        );
        if (!m_window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        // Create GL context
        m_glContext = SDL_GL_CreateContext(m_window);
        if (!m_glContext) {
            std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
            TTF_Quit();
            SDL_Quit();
            return false;
        }

        // Load OpenGL functions
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

        // Initial GL state
        glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        glEnable(GL_DEPTH_TEST);

        m_initialized = true;
        return true;
    }

    // Clean up all resources
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

    // Swap buffers
    void swapBuffers() {
        if (m_window) {
            SDL_GL_SwapWindow(m_window);
        }
    }

    // Accessors
    SDL_Window* window() const { return m_window; }
    SDL_GLContext glContext() const { return m_glContext; }
    bool isInitialized() const { return m_initialized; }

    // Convenience accessors using GameConfig
    static constexpr int width() { return GameConfig::WINDOW_WIDTH; }
    static constexpr int height() { return GameConfig::WINDOW_HEIGHT; }
    static constexpr float aspectRatio() {
        return static_cast<float>(GameConfig::WINDOW_WIDTH) / static_cast<float>(GameConfig::WINDOW_HEIGHT);
    }

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    bool m_initialized = false;
};
