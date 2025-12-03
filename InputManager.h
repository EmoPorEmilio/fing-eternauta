/**
 * @file InputManager.h
 * @brief SDL event processing and input state management
 *
 * InputManager processes all SDL events and publishes typed events via
 * EventBus. It maintains keyboard and mouse state for both event-driven
 * and polling-based input handling.
 *
 * Event Processing Flow:
 *   SDL_PollEvent() -> InputManager::processEvents() -> EventBus::publish()
 *
 * Published Events:
 *   - KeyPressedEvent / KeyReleasedEvent
 *   - MouseButtonPressedEvent / MouseButtonReleasedEvent
 *   - MouseMovedEvent (with delta for camera look)
 *   - MouseScrolledEvent
 *   - WindowResizedEvent / WindowClosedEvent / WindowFocusEvent
 *
 * State Queries:
 *   - isKeyDown(scancode): Currently held
 *   - isKeyPressed(scancode): Just pressed this frame
 *   - isKeyReleased(scancode): Just released this frame
 *   - Same pattern for mouse buttons
 *
 * ImGui Integration:
 *   InputManager checks ImGui::GetIO().WantCaptureKeyboard/Mouse to
 *   avoid processing input when ImGui has focus. Set via:
 *   - setImGuiWantsKeyboard(bool)
 *   - setImGuiWantsMouse(bool)
 *
 * Cursor Capture:
 *   - setCursorCaptured(true): Hides cursor, enables relative mouse mode
 *   - Used for FPS-style camera look when right mouse button held
 *
 * Event Preprocessor:
 *   Set a callback via setEventPreprocessor() to let ImGui process
 *   events before InputManager (typically ImGui_ImplSDL2_ProcessEvent).
 *
 * @see events/InputEvents.h, Application, Camera
 */
#pragma once

#include <SDL.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <functional>
#include "events/Events.h"

class InputManager {
public:
    static InputManager& instance();

    // Initialize with SDL window reference
    void initialize(SDL_Window* window);
    void shutdown();

    // Call once per frame to process all pending SDL events
    // Returns false if application should quit
    bool processEvents();

    // Keyboard state queries
    bool isKeyDown(SDL_Scancode scancode) const;
    bool isKeyPressed(SDL_Scancode scancode) const;  // Just pressed this frame
    bool isKeyReleased(SDL_Scancode scancode) const; // Just released this frame

    // Mouse state queries
    bool isMouseButtonDown(int button) const;
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonReleased(int button) const;
    glm::vec2 getMousePosition() const;
    glm::vec2 getMouseDelta() const;

    // Cursor capture control
    void setCursorCaptured(bool captured);
    bool isCursorCaptured() const { return m_cursorCaptured; }

    // ImGui integration - set this so InputManager knows when to ignore input
    void setImGuiWantsKeyboard(bool wants) { m_imguiWantsKeyboard = wants; }
    void setImGuiWantsMouse(bool wants) { m_imguiWantsMouse = wants; }
    bool doesImGuiWantKeyboard() const { return m_imguiWantsKeyboard; }
    bool doesImGuiWantMouse() const { return m_imguiWantsMouse; }

    // Get raw keyboard state pointer for legacy camera code
    const Uint8* getKeyboardState() const { return SDL_GetKeyboardState(nullptr); }

    // Event preprocessor - called for each SDL event before InputManager processes it
    // Use this for ImGui integration: set to a function that calls ImGui_ImplSDL2_ProcessEvent
    using EventPreprocessor = std::function<void(const SDL_Event&)>;
    void setEventPreprocessor(EventPreprocessor preprocessor) { m_eventPreprocessor = preprocessor; }

private:
    InputManager() = default;
    ~InputManager() = default;
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    // Convert SDL keycode to our KeyCode enum
    events::KeyCode sdlKeyToKeyCode(SDL_Keycode sdlKey) const;

    // Process individual event types
    void processKeyEvent(const SDL_Event& event);
    void processMouseButtonEvent(const SDL_Event& event);
    void processMouseMotionEvent(const SDL_Event& event);
    void processMouseWheelEvent(const SDL_Event& event);
    void processWindowEvent(const SDL_Event& event);

    SDL_Window* m_window = nullptr;
    bool m_initialized = false;

    // Cursor state
    bool m_cursorCaptured = false;
    bool m_rightMouseHeld = false;

    // Mouse state
    glm::vec2 m_mousePosition = glm::vec2(0.0f);
    glm::vec2 m_mouseDelta = glm::vec2(0.0f);
    std::unordered_map<int, bool> m_mouseButtonsDown;
    std::unordered_map<int, bool> m_mouseButtonsPressed;
    std::unordered_map<int, bool> m_mouseButtonsReleased;

    // Keyboard state tracking (for pressed/released detection)
    std::unordered_map<SDL_Scancode, bool> m_keysDown;
    std::unordered_map<SDL_Scancode, bool> m_keysPressed;
    std::unordered_map<SDL_Scancode, bool> m_keysReleased;

    // ImGui integration
    bool m_imguiWantsKeyboard = false;
    bool m_imguiWantsMouse = false;

    // Event preprocessor for ImGui
    EventPreprocessor m_eventPreprocessor = nullptr;
};
