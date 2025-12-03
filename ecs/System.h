#pragma once

#include "Registry.h"
#include <vector>
#include <memory>
#include <string>

namespace ecs {

// Base system interface
class System {
public:
    virtual ~System() = default;

    // Called once when system is added to scheduler
    virtual void init(Registry& registry) { (void)registry; }

    // Called every frame
    virtual void update(Registry& registry, float deltaTime) = 0;

    // System name for debugging
    virtual const char* name() const = 0;

    // Enable/disable system
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

protected:
    bool m_enabled = true;
};

// System scheduler - runs systems in registration order
class SystemScheduler {
public:
    SystemScheduler() = default;
    ~SystemScheduler() = default;

    // Add a system (takes ownership)
    template<typename T, typename... Args>
    T& addSystem(Args&&... args) {
        static_assert(std::is_base_of_v<System, T>, "T must derive from System");
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *system;
        m_systems.push_back(std::move(system));
        return ref;
    }

    // Initialize all systems
    void init(Registry& registry) {
        for (auto& system : m_systems) {
            system->init(registry);
        }
    }

    // Update all enabled systems
    void update(Registry& registry, float deltaTime) {
        for (auto& system : m_systems) {
            if (system->isEnabled()) {
                system->update(registry, deltaTime);
            }
        }
    }

    // Get system by type
    template<typename T>
    T* getSystem() {
        for (auto& system : m_systems) {
            if (auto* casted = dynamic_cast<T*>(system.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    // Get all systems
    const std::vector<std::unique_ptr<System>>& getSystems() const { return m_systems; }

private:
    std::vector<std::unique_ptr<System>> m_systems;
};

} // namespace ecs
