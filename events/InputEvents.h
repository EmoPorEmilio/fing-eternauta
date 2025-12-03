#pragma once

#include "Event.h"
#include <glm/glm.hpp>

namespace events {

// Key codes (subset of SDL keycodes for common keys)
enum class KeyCode {
    Unknown = 0,
    // Letters
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    // Numbers
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    // Special keys
    Space, Escape, Enter, Tab, Backspace, Delete, Insert,
    Up, Down, Left, Right,
    Home, End, PageUp, PageDown,
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
    // Misc
    Grave, Minus, Equals, LeftBracket, RightBracket,
    Backslash, Semicolon, Apostrophe, Comma, Period, Slash
};

// Mouse button codes
enum class MouseButton {
    Left = 0,
    Middle = 1,
    Right = 2,
    X1 = 3,
    X2 = 4
};

// Key pressed event
struct KeyPressedEvent : public EventBase<KeyPressedEvent> {
    KeyCode key;
    int scancode;
    bool repeat;
    bool shift;
    bool ctrl;
    bool alt;

    KeyPressedEvent(KeyCode k, int sc, bool r = false, bool s = false, bool c = false, bool a = false)
        : key(k), scancode(sc), repeat(r), shift(s), ctrl(c), alt(a) {}
};

// Key released event
struct KeyReleasedEvent : public EventBase<KeyReleasedEvent> {
    KeyCode key;
    int scancode;

    KeyReleasedEvent(KeyCode k, int sc) : key(k), scancode(sc) {}
};

// Mouse moved event
struct MouseMovedEvent : public EventBase<MouseMovedEvent> {
    float x;
    float y;
    float deltaX;
    float deltaY;

    MouseMovedEvent(float px, float py, float dx, float dy)
        : x(px), y(py), deltaX(dx), deltaY(dy) {}
};

// Mouse button pressed event
struct MouseButtonPressedEvent : public EventBase<MouseButtonPressedEvent> {
    MouseButton button;
    float x;
    float y;

    MouseButtonPressedEvent(MouseButton b, float px, float py)
        : button(b), x(px), y(py) {}
};

// Mouse button released event
struct MouseButtonReleasedEvent : public EventBase<MouseButtonReleasedEvent> {
    MouseButton button;
    float x;
    float y;

    MouseButtonReleasedEvent(MouseButton b, float px, float py)
        : button(b), x(px), y(py) {}
};

// Mouse wheel scrolled event
struct MouseScrolledEvent : public EventBase<MouseScrolledEvent> {
    float offsetX;
    float offsetY;

    MouseScrolledEvent(float ox, float oy) : offsetX(ox), offsetY(oy) {}
};

// High-level action events (abstracted from raw input)
struct FlashlightToggleEvent : public EventBase<FlashlightToggleEvent> {
    FlashlightToggleEvent() = default;
};

struct CameraMoveRequestEvent : public EventBase<CameraMoveRequestEvent> {
    glm::vec3 direction; // Normalized direction
    float speed;

    CameraMoveRequestEvent(const glm::vec3& dir, float s) : direction(dir), speed(s) {}
};

struct CameraLookRequestEvent : public EventBase<CameraLookRequestEvent> {
    float deltaYaw;
    float deltaPitch;

    CameraLookRequestEvent(float dy, float dp) : deltaYaw(dy), deltaPitch(dp) {}
};

} // namespace events
