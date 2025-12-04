#pragma once

#include "Event.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <algorithm>

namespace events {

// Subscription handle for unsubscribing
using SubscriptionId = uint32_t;

// Internal wrapper for type-erased handlers
class IEventHandlerWrapper {
public:
    virtual ~IEventHandlerWrapper() = default;
    virtual void call(const Event& event) = 0;
    virtual std::type_index getEventType() const = 0;
};

template<typename EventType>
class EventHandlerWrapper : public IEventHandlerWrapper {
public:
    explicit EventHandlerWrapper(std::function<void(const EventType&)> handler)
        : m_handler(std::move(handler)) {}

    void call(const Event& event) override {
        m_handler(static_cast<const EventType&>(event));
    }

    std::type_index getEventType() const override {
        return EventType::staticType();
    }

private:
    std::function<void(const EventType&)> m_handler;
};

// Central event bus - singleton for global event dispatch
class EventBus {
public:
    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }

    // Subscribe to an event type with a handler function
    // Returns a subscription ID that can be used to unsubscribe
    template<typename EventType>
    SubscriptionId subscribe(std::function<void(const EventType&)> handler) {
        auto wrapper = std::make_unique<EventHandlerWrapper<EventType>>(std::move(handler));
        auto typeIndex = EventType::staticType();

        SubscriptionId id = m_nextId++;

        m_handlers[typeIndex].push_back({id, std::move(wrapper)});

        return id;
    }

    // Subscribe with a member function
    template<typename EventType, typename T>
    SubscriptionId subscribe(T* instance, void (T::*memberFunc)(const EventType&)) {
        return subscribe<EventType>([instance, memberFunc](const EventType& event) {
            (instance->*memberFunc)(event);
        });
    }

    // Unsubscribe using subscription ID
    void unsubscribe(SubscriptionId id) {
        for (auto& [typeIndex, handlers] : m_handlers) {
            handlers.erase(
                std::remove_if(handlers.begin(), handlers.end(),
                    [id](const HandlerEntry& entry) { return entry.id == id; }),
                handlers.end()
            );
        }
    }

    // Publish an event to all subscribers
    template<typename EventType>
    void publish(const EventType& event) {
        auto typeIndex = EventType::staticType();

        auto it = m_handlers.find(typeIndex);
        if (it == m_handlers.end()) {
            return;
        }

        for (auto& entry : it->second) {
            entry.handler->call(event);
            if (event.handled) {
                break; // Stop propagation if handled
            }
        }
    }

    // Publish with move semantics for temporary events
    template<typename EventType>
    void publish(EventType&& event) {
        publish(static_cast<const EventType&>(event));
    }

    // Queue an event for deferred processing
    template<typename EventType>
    void queue(const EventType& event) {
        m_eventQueue.push_back(std::make_unique<EventType>(event));
    }

    // Process all queued events
    void processQueue() {
        // Swap to avoid issues with events queuing more events
        std::vector<std::unique_ptr<Event>> eventsToProcess;
        eventsToProcess.swap(m_eventQueue);

        for (auto& event : eventsToProcess) {
            auto typeIndex = event->getType();
            auto it = m_handlers.find(typeIndex);
            if (it != m_handlers.end()) {
                for (auto& entry : it->second) {
                    entry.handler->call(*event);
                    if (event->handled) {
                        break;
                    }
                }
            }
        }
    }

    // Clear all subscriptions (useful for cleanup)
    void clear() {
        m_handlers.clear();
        m_eventQueue.clear();
        m_nextId = 1;
    }

    // Get subscriber count for an event type (for debugging)
    template<typename EventType>
    size_t getSubscriberCount() const {
        auto typeIndex = EventType::staticType();
        auto it = m_handlers.find(typeIndex);
        return (it != m_handlers.end()) ? it->second.size() : 0;
    }

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    struct HandlerEntry {
        SubscriptionId id;
        std::unique_ptr<IEventHandlerWrapper> handler;
    };

    std::unordered_map<std::type_index, std::vector<HandlerEntry>> m_handlers;
    std::vector<std::unique_ptr<Event>> m_eventQueue;
    SubscriptionId m_nextId = 1;
};

// Convenience macro for subscribing member functions
#define SUBSCRIBE_EVENT(EventType, instance, memberFunc) \
    events::EventBus::instance().subscribe<EventType>(instance, &std::remove_pointer_t<decltype(instance)>::memberFunc)

// Convenience function for publishing events
template<typename EventType, typename... Args>
void emit(Args&&... args) {
    EventBus::instance().publish(EventType{std::forward<Args>(args)...});
}

} // namespace events
