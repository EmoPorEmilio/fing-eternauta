#pragma once

#include <cstdint>
#include <typeindex>
#include <functional>

namespace events {

// Base class for all events - provides type info for runtime dispatch
class Event {
public:
    virtual ~Event() = default;

    // Returns the type index for this event type
    virtual std::type_index getType() const = 0;

    // Whether this event has been handled (stops propagation if true)
    bool handled = false;
};

// CRTP helper for automatic type implementation
template<typename Derived>
class EventBase : public Event {
public:
    static std::type_index staticType() {
        return std::type_index(typeid(Derived));
    }

    std::type_index getType() const override {
        return staticType();
    }
};

// Type alias for event handlers
template<typename EventType>
using EventHandler = std::function<void(const EventType&)>;

} // namespace events
