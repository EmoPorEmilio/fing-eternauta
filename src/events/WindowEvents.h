#pragma once

#include "Event.h"

namespace events {

// Window resized event
struct WindowResizedEvent : public EventBase<WindowResizedEvent> {
    int width;
    int height;

    WindowResizedEvent(int w, int h) : width(w), height(h) {}
};

// Window closed event
struct WindowClosedEvent : public EventBase<WindowClosedEvent> {
    WindowClosedEvent() = default;
};

// Window focus events
struct WindowFocusEvent : public EventBase<WindowFocusEvent> {
    bool focused;

    explicit WindowFocusEvent(bool f) : focused(f) {}
};

// Window minimized/maximized
struct WindowMinimizedEvent : public EventBase<WindowMinimizedEvent> {
    bool minimized;

    explicit WindowMinimizedEvent(bool m) : minimized(m) {}
};

} // namespace events
