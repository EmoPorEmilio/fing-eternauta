#include "InputManager.h"
#include <iostream>

InputManager& InputManager::instance() {
    static InputManager manager;
    return manager;
}

void InputManager::initialize(SDL_Window* window) {
    m_window = window;
    m_initialized = true;

    // Start with cursor free (not captured)
    SDL_SetRelativeMouseMode(SDL_FALSE);
    m_cursorCaptured = false;

    std::cout << "[InputManager] Initialized" << std::endl;
}

void InputManager::shutdown() {
    m_initialized = false;
    m_window = nullptr;
    std::cout << "[InputManager] Shutdown" << std::endl;
}

bool InputManager::processEvents() {
    if (!m_initialized) return true;

    // Clear per-frame state
    m_keysPressed.clear();
    m_keysReleased.clear();
    m_mouseButtonsPressed.clear();
    m_mouseButtonsReleased.clear();
    m_mouseDelta = glm::vec2(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // Call preprocessor first (for ImGui integration)
        if (m_eventPreprocessor) {
            m_eventPreprocessor(event);
        }

        switch (event.type) {
            case SDL_QUIT:
                events::EventBus::instance().publish(events::WindowClosedEvent());
                return false;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                processKeyEvent(event);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                processMouseButtonEvent(event);
                break;

            case SDL_MOUSEMOTION:
                processMouseMotionEvent(event);
                break;

            case SDL_MOUSEWHEEL:
                processMouseWheelEvent(event);
                break;

            case SDL_WINDOWEVENT:
                processWindowEvent(event);
                break;
        }
    }

    return true;
}

void InputManager::processKeyEvent(const SDL_Event& event) {
    bool isDown = (event.type == SDL_KEYDOWN);
    SDL_Scancode scancode = event.key.keysym.scancode;
    SDL_Keycode keycode = event.key.keysym.sym;
    bool repeat = event.key.repeat != 0;

    // Update key state
    bool wasDown = m_keysDown[scancode];
    m_keysDown[scancode] = isDown;

    if (isDown && !wasDown) {
        m_keysPressed[scancode] = true;
    } else if (!isDown && wasDown) {
        m_keysReleased[scancode] = true;
    }

    // Get modifier state
    Uint16 mod = event.key.keysym.mod;
    bool shift = (mod & KMOD_SHIFT) != 0;
    bool ctrl = (mod & KMOD_CTRL) != 0;
    bool alt = (mod & KMOD_ALT) != 0;

    // Convert to our KeyCode
    events::KeyCode keyCode = sdlKeyToKeyCode(keycode);

    // Publish events (only if ImGui doesn't want keyboard)
    if (!m_imguiWantsKeyboard) {
        if (isDown) {
            events::EventBus::instance().publish(
                events::KeyPressedEvent(keyCode, scancode, repeat, shift, ctrl, alt));

            // High-level action events
            if (keycode == SDLK_SPACE && !repeat) {
                events::EventBus::instance().publish(events::FlashlightToggleEvent());
            }
        } else {
            events::EventBus::instance().publish(
                events::KeyReleasedEvent(keyCode, scancode));
        }
    }
}

void InputManager::processMouseButtonEvent(const SDL_Event& event) {
    bool isDown = (event.type == SDL_MOUSEBUTTONDOWN);
    int button = event.button.button;
    float x = static_cast<float>(event.button.x);
    float y = static_cast<float>(event.button.y);

    // Update button state
    bool wasDown = m_mouseButtonsDown[button];
    m_mouseButtonsDown[button] = isDown;

    if (isDown && !wasDown) {
        m_mouseButtonsPressed[button] = true;
    } else if (!isDown && wasDown) {
        m_mouseButtonsReleased[button] = true;
    }

    // Right mouse button controls camera
    if (button == SDL_BUTTON_RIGHT) {
        m_rightMouseHeld = isDown;
        setCursorCaptured(isDown);
    }

    // Convert to our MouseButton enum
    events::MouseButton mouseBtn;
    switch (button) {
        case SDL_BUTTON_LEFT: mouseBtn = events::MouseButton::Left; break;
        case SDL_BUTTON_MIDDLE: mouseBtn = events::MouseButton::Middle; break;
        case SDL_BUTTON_RIGHT: mouseBtn = events::MouseButton::Right; break;
        case SDL_BUTTON_X1: mouseBtn = events::MouseButton::X1; break;
        case SDL_BUTTON_X2: mouseBtn = events::MouseButton::X2; break;
        default: return;
    }

    // Publish events (only if ImGui doesn't want mouse)
    if (!m_imguiWantsMouse) {
        if (isDown) {
            events::EventBus::instance().publish(
                events::MouseButtonPressedEvent(mouseBtn, x, y));
        } else {
            events::EventBus::instance().publish(
                events::MouseButtonReleasedEvent(mouseBtn, x, y));
        }
    }
}

void InputManager::processMouseMotionEvent(const SDL_Event& event) {
    m_mousePosition.x = static_cast<float>(event.motion.x);
    m_mousePosition.y = static_cast<float>(event.motion.y);

    float deltaX = static_cast<float>(event.motion.xrel);
    float deltaY = static_cast<float>(event.motion.yrel);
    m_mouseDelta.x += deltaX;
    m_mouseDelta.y += deltaY;

    // Publish event (camera control usually only when right mouse held)
    if (m_rightMouseHeld && !m_imguiWantsMouse) {
        events::EventBus::instance().publish(
            events::MouseMovedEvent(m_mousePosition.x, m_mousePosition.y, deltaX, deltaY));

        // High-level camera look request
        events::EventBus::instance().publish(
            events::CameraLookRequestEvent(deltaX, -deltaY));
    }
}

void InputManager::processMouseWheelEvent(const SDL_Event& event) {
    float offsetX = static_cast<float>(event.wheel.x);
    float offsetY = static_cast<float>(event.wheel.y);

    if (!m_imguiWantsMouse) {
        events::EventBus::instance().publish(
            events::MouseScrolledEvent(offsetX, offsetY));
    }
}

void InputManager::processWindowEvent(const SDL_Event& event) {
    switch (event.window.event) {
        case SDL_WINDOWEVENT_SIZE_CHANGED:
        case SDL_WINDOWEVENT_RESIZED: {
            int w = 0, h = 0;
            if (m_window) {
                SDL_GL_GetDrawableSize(m_window, &w, &h);
            }
            events::EventBus::instance().publish(
                events::WindowResizedEvent(w, h));
            break;
        }

        case SDL_WINDOWEVENT_FOCUS_GAINED:
            events::EventBus::instance().publish(events::WindowFocusEvent(true));
            break;

        case SDL_WINDOWEVENT_FOCUS_LOST:
            events::EventBus::instance().publish(events::WindowFocusEvent(false));
            break;

        case SDL_WINDOWEVENT_MINIMIZED:
            events::EventBus::instance().publish(events::WindowMinimizedEvent(true));
            break;

        case SDL_WINDOWEVENT_RESTORED:
            events::EventBus::instance().publish(events::WindowMinimizedEvent(false));
            break;
    }
}

// Keyboard state queries
bool InputManager::isKeyDown(SDL_Scancode scancode) const {
    auto it = m_keysDown.find(scancode);
    return (it != m_keysDown.end()) ? it->second : false;
}

bool InputManager::isKeyPressed(SDL_Scancode scancode) const {
    auto it = m_keysPressed.find(scancode);
    return (it != m_keysPressed.end()) ? it->second : false;
}

bool InputManager::isKeyReleased(SDL_Scancode scancode) const {
    auto it = m_keysReleased.find(scancode);
    return (it != m_keysReleased.end()) ? it->second : false;
}

// Mouse state queries
bool InputManager::isMouseButtonDown(int button) const {
    auto it = m_mouseButtonsDown.find(button);
    return (it != m_mouseButtonsDown.end()) ? it->second : false;
}

bool InputManager::isMouseButtonPressed(int button) const {
    auto it = m_mouseButtonsPressed.find(button);
    return (it != m_mouseButtonsPressed.end()) ? it->second : false;
}

bool InputManager::isMouseButtonReleased(int button) const {
    auto it = m_mouseButtonsReleased.find(button);
    return (it != m_mouseButtonsReleased.end()) ? it->second : false;
}

glm::vec2 InputManager::getMousePosition() const {
    return m_mousePosition;
}

glm::vec2 InputManager::getMouseDelta() const {
    return m_mouseDelta;
}

void InputManager::setCursorCaptured(bool captured) {
    m_cursorCaptured = captured;
    SDL_SetRelativeMouseMode(captured ? SDL_TRUE : SDL_FALSE);
}

events::KeyCode InputManager::sdlKeyToKeyCode(SDL_Keycode sdlKey) const {
    switch (sdlKey) {
        // Letters
        case SDLK_a: return events::KeyCode::A;
        case SDLK_b: return events::KeyCode::B;
        case SDLK_c: return events::KeyCode::C;
        case SDLK_d: return events::KeyCode::D;
        case SDLK_e: return events::KeyCode::E;
        case SDLK_f: return events::KeyCode::F;
        case SDLK_g: return events::KeyCode::G;
        case SDLK_h: return events::KeyCode::H;
        case SDLK_i: return events::KeyCode::I;
        case SDLK_j: return events::KeyCode::J;
        case SDLK_k: return events::KeyCode::K;
        case SDLK_l: return events::KeyCode::L;
        case SDLK_m: return events::KeyCode::M;
        case SDLK_n: return events::KeyCode::N;
        case SDLK_o: return events::KeyCode::O;
        case SDLK_p: return events::KeyCode::P;
        case SDLK_q: return events::KeyCode::Q;
        case SDLK_r: return events::KeyCode::R;
        case SDLK_s: return events::KeyCode::S;
        case SDLK_t: return events::KeyCode::T;
        case SDLK_u: return events::KeyCode::U;
        case SDLK_v: return events::KeyCode::V;
        case SDLK_w: return events::KeyCode::W;
        case SDLK_x: return events::KeyCode::X;
        case SDLK_y: return events::KeyCode::Y;
        case SDLK_z: return events::KeyCode::Z;

        // Numbers
        case SDLK_0: return events::KeyCode::Num0;
        case SDLK_1: return events::KeyCode::Num1;
        case SDLK_2: return events::KeyCode::Num2;
        case SDLK_3: return events::KeyCode::Num3;
        case SDLK_4: return events::KeyCode::Num4;
        case SDLK_5: return events::KeyCode::Num5;
        case SDLK_6: return events::KeyCode::Num6;
        case SDLK_7: return events::KeyCode::Num7;
        case SDLK_8: return events::KeyCode::Num8;
        case SDLK_9: return events::KeyCode::Num9;

        // Function keys
        case SDLK_F1: return events::KeyCode::F1;
        case SDLK_F2: return events::KeyCode::F2;
        case SDLK_F3: return events::KeyCode::F3;
        case SDLK_F4: return events::KeyCode::F4;
        case SDLK_F5: return events::KeyCode::F5;
        case SDLK_F6: return events::KeyCode::F6;
        case SDLK_F7: return events::KeyCode::F7;
        case SDLK_F8: return events::KeyCode::F8;
        case SDLK_F9: return events::KeyCode::F9;
        case SDLK_F10: return events::KeyCode::F10;
        case SDLK_F11: return events::KeyCode::F11;
        case SDLK_F12: return events::KeyCode::F12;

        // Special keys
        case SDLK_SPACE: return events::KeyCode::Space;
        case SDLK_ESCAPE: return events::KeyCode::Escape;
        case SDLK_RETURN: return events::KeyCode::Enter;
        case SDLK_TAB: return events::KeyCode::Tab;
        case SDLK_BACKSPACE: return events::KeyCode::Backspace;
        case SDLK_DELETE: return events::KeyCode::Delete;
        case SDLK_INSERT: return events::KeyCode::Insert;

        // Arrow keys
        case SDLK_UP: return events::KeyCode::Up;
        case SDLK_DOWN: return events::KeyCode::Down;
        case SDLK_LEFT: return events::KeyCode::Left;
        case SDLK_RIGHT: return events::KeyCode::Right;

        // Navigation
        case SDLK_HOME: return events::KeyCode::Home;
        case SDLK_END: return events::KeyCode::End;
        case SDLK_PAGEUP: return events::KeyCode::PageUp;
        case SDLK_PAGEDOWN: return events::KeyCode::PageDown;

        // Modifiers
        case SDLK_LSHIFT: return events::KeyCode::LeftShift;
        case SDLK_RSHIFT: return events::KeyCode::RightShift;
        case SDLK_LCTRL: return events::KeyCode::LeftCtrl;
        case SDLK_RCTRL: return events::KeyCode::RightCtrl;
        case SDLK_LALT: return events::KeyCode::LeftAlt;
        case SDLK_RALT: return events::KeyCode::RightAlt;

        default: return events::KeyCode::Unknown;
    }
}
