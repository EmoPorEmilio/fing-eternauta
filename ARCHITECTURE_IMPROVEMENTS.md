# Architecture Improvements Plan

## Current State Summary

The ECS implementation is functional with:
- 11 component types (Transform, MeshGroup, Skeleton, Animation, Renderable, Camera, RigidBody, GroundPlane, BoxCollider, PlayerController, FollowTarget)
- 8 systems (Input, PlayerMovement, CameraOrbit, FollowCamera, Animation, Skeleton, Physics, Collision, Render)
- Working 3rd person camera with over-shoulder view
- Skeletal animation with walk/run/backrun states
- Basic collision/physics

---

## Pain Points Identified

### 1. Registry Scalability
**Problem:** Every new component requires 6 touch points in Registry.h:
- Add `std::unordered_map<Entity, ComponentType>`
- Add `addComponentType()` method
- Add `getComponentType()` method
- Add `forEachComponentType()` method
- Update `destroy()` to erase from new map

**Impact:** Adding more components (AI, Inventory, Health, etc.) will make Registry bloated and error-prone.

### 2. No Generic Queries
**Problem:** Each component combination needs a dedicated `forEach*()` method. Can't easily query "all entities with Transform + RigidBody + NOT BoxCollider".

**Example of current verbosity:**
```cpp
// Each of these is a separate method
forEachRenderable()      // Transform + MeshGroup + Renderable
forEachAnimated()        // Animation + Skeleton
forEachPlayerController() // Transform + PlayerController
forEachFollowTarget()    // Transform + FollowTarget
// ...and growing
```

### 3. Animation Clips Stored Outside Registry
**Problem:** `AnimationSystem` holds `m_clips` map internally. Clips should be a component or part of the entity.

**Current (in AnimationSystem.h):**
```cpp
class AnimationSystem {
    std::unordered_map<Entity, std::vector<AnimationClip>> m_clips;  // BAD: state in system
public:
    void setClips(Entity entity, std::vector<AnimationClip> clips);
};
```

### 4. FollowTarget Does Two Jobs
**Problem:** `FollowTarget` manages both camera position AND character facing direction via `yaw`. This couples camera and player systems.

**Current (in FollowTarget.h):**
```cpp
struct FollowTarget {
    Entity target;
    float distance, height, shoulderOffset, lookAhead;  // Camera positioning
    float yaw, pitch;    // Shared: camera orbit AND character facing
    float sensitivity;   // Input sensitivity
};
```

### 5. No Event/Message System
**Problem:** Components talk through shared state. No way to fire "entity died" or "animation finished" events.

### 6. Entity Lifecycle Gaps
**Problem:** No `isAlive(Entity)` check. No `hasComponent<T>(Entity)` helper. Destroyed entities leave dangling references.

---

## Improvement Priorities

### Priority 1: Quick Wins (Low effort, high value)

#### 1a. Add `has*()` Component Checks
```cpp
// Add to Registry.h
bool hasTransform(Entity e) const { return m_transforms.count(e) > 0; }
bool hasSkeleton(Entity e) const { return m_skeletons.count(e) > 0; }
bool hasAnimation(Entity e) const { return m_animations.count(e) > 0; }
// ... for all components

// Or a generic version:
template<typename T>
bool has(Entity e) const;
```

#### 1b. Add `isAlive(Entity)` Check
```cpp
// Track alive entities
std::unordered_set<Entity> m_alive;

Entity create() {
    Entity e = ++m_nextId;
    m_alive.insert(e);
    return e;
}

void destroy(Entity e) {
    m_alive.erase(e);
    // ... erase from all maps
}

bool isAlive(Entity e) const {
    return m_alive.count(e) > 0;
}
```

#### 1c. Move Animation Clips to Component
```cpp
// In Animation.h
struct Animation {
    int clipIndex = 0;
    float time = 0.0f;
    bool playing = false;
    std::vector<AnimationClip> clips;  // NEW: clips live here now
};

// AnimationSystem becomes stateless
class AnimationSystem {
public:
    void update(Registry& registry, float dt) {
        registry.forEachAnimated([&](Entity entity, Animation& anim, Skeleton& skeleton) {
            if (!anim.playing) return;
            if (anim.clipIndex < 0 || anim.clipIndex >= (int)anim.clips.size()) return;
            const AnimationClip& clip = anim.clips[anim.clipIndex];
            // ... rest unchanged
        });
    }
    // No more m_clips member!
};
```

**main.cpp change:**
```cpp
// Before:
animationSystem.setClips(protagonist, std::move(protagonistData.clips));
Animation anim;
registry.addAnimation(protagonist, anim);

// After:
Animation anim;
anim.clips = std::move(protagonistData.clips);
registry.addAnimation(protagonist, anim);
```

---

### Priority 2: Split FollowTarget Responsibility

#### 2a. Create FacingDirection Component
```cpp
// src/ecs/components/FacingDirection.h
#pragma once

struct FacingDirection {
    float yaw = 0.0f;      // Horizontal angle in degrees
    float turnSpeed = 10.0f;  // Smoothing factor
};
```

#### 2b. Simplify FollowTarget (Camera Only)
```cpp
// src/ecs/components/FollowTarget.h
#pragma once
#include "../Entity.h"

struct FollowTarget {
    Entity target = NULL_ENTITY;
    float distance = 2.2f;
    float height = 1.2f;
    float shoulderOffset = 2.4f;
    float lookAhead = 5.0f;
    float pitch = 10.0f;
    float sensitivity = 0.3f;
    // yaw REMOVED - comes from target's FacingDirection
};
```

#### 2c. Create MouseInputReceiver Component
```cpp
// src/ecs/components/MouseInputReceiver.h
#pragma once

struct MouseInputReceiver {
    float sensitivity = 0.3f;
    bool invertY = false;
};
```

#### 2d. Update Systems

**CameraOrbitSystem** - reads mouse, writes to FacingDirection on player:
```cpp
void update(Registry& registry, int mouseX, int mouseY) {
    registry.forEachFollowTarget([&](Entity camEntity, Transform&, FollowTarget& ft) {
        if (ft.target == NULL_ENTITY) return;

        // Get target's facing direction
        auto* facing = registry.getFacingDirection(ft.target);
        if (!facing) return;

        // Mouse updates player's facing, not camera
        facing->yaw -= mouseX * ft.sensitivity;
        ft.pitch += mouseY * ft.sensitivity;
        ft.pitch = glm::clamp(ft.pitch, -45.0f, 60.0f);
    });
}
```

**PlayerMovementSystem** - reads from own FacingDirection:
```cpp
void update(Registry& registry, float dt) {
    registry.forEachPlayerController([&](Entity entity, Transform& transform, PlayerController& pc) {
        auto* facing = registry.getFacingDirection(entity);
        if (!facing) return;

        float yawRad = glm::radians(facing->yaw);
        // ... movement logic using yawRad

        // Rotation smoothing
        glm::quat targetRot = glm::angleAxis(yawRad + glm::pi<float>(), glm::vec3(0,1,0));
        transform.rotation = glm::slerp(transform.rotation, targetRot, facing->turnSpeed * dt);
    });
}
```

**FollowCameraSystem** - reads target's FacingDirection for yaw:
```cpp
void update(Registry& registry) {
    registry.forEachFollowTarget([&](Entity camEntity, Transform& camTransform, FollowTarget& ft) {
        if (ft.target == NULL_ENTITY) return;
        auto* targetTransform = registry.getTransform(ft.target);
        auto* facing = registry.getFacingDirection(ft.target);
        if (!targetTransform || !facing) return;

        float yawRad = glm::radians(facing->yaw);
        glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,1,0)));

        camTransform.position = targetTransform->position
            - forward * ft.distance
            + right * ft.shoulderOffset;
        camTransform.position.y += ft.height;
    });
}
```

**RenderSystem** - also reads target's FacingDirection:
```cpp
// In the camera view calculation
auto* facing = registry.getFacingDirection(followTarget->target);
if (facing) {
    float yawRad = glm::radians(facing->yaw);
    glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
    glm::vec3 lookAtPos = targetTransform->position + forward * followTarget->lookAhead;
    lookAtPos.y += 1.0f;
    view = glm::lookAt(camTransform->position, lookAtPos, glm::vec3(0,1,0));
}
```

**main.cpp setup changes:**
```cpp
// Before:
FollowTarget followTarget;
followTarget.target = protagonist;
registry.addFollowTarget(camera, followTarget);
float cameraYaw = ft ? ft->yaw : 0.0f;  // Getting yaw from camera
playerMovementSystem.update(registry, dt, cameraYaw);

// After:
FacingDirection facing;
registry.addFacingDirection(protagonist, facing);  // Player has its own facing

FollowTarget followTarget;
followTarget.target = protagonist;
registry.addFollowTarget(camera, followTarget);

// No more passing cameraYaw - player reads its own FacingDirection
playerMovementSystem.update(registry, dt);
```

---

### Priority 3: Generic Component System (Medium effort)

#### 3a. Type-Erased Component Storage
```cpp
// src/ecs/ComponentPool.h
#pragma once
#include "Entity.h"
#include <unordered_map>
#include <any>
#include <typeindex>

class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(Entity e) = 0;
    virtual bool contains(Entity e) const = 0;
};

template<typename T>
class ComponentPool : public IComponentPool {
public:
    T& add(Entity e, T component = {}) {
        m_data[e] = std::move(component);
        return m_data[e];
    }

    T* get(Entity e) {
        auto it = m_data.find(e);
        return it != m_data.end() ? &it->second : nullptr;
    }

    void remove(Entity e) override { m_data.erase(e); }
    bool contains(Entity e) const override { return m_data.count(e) > 0; }

    auto begin() { return m_data.begin(); }
    auto end() { return m_data.end(); }

private:
    std::unordered_map<Entity, T> m_data;
};
```

#### 3b. Updated Registry with Type-Erased Pools
```cpp
// src/ecs/Registry.h
#pragma once
#include "Entity.h"
#include "ComponentPool.h"
#include <unordered_map>
#include <unordered_set>
#include <typeindex>
#include <memory>

class Registry {
public:
    Entity create() {
        Entity e = ++m_nextId;
        m_alive.insert(e);
        return e;
    }

    void destroy(Entity e) {
        m_alive.erase(e);
        for (auto& [type, pool] : m_pools) {
            pool->remove(e);
        }
    }

    bool isAlive(Entity e) const {
        return m_alive.count(e) > 0;
    }

    template<typename T>
    T& add(Entity e, T component = {}) {
        return getPool<T>().add(e, std::move(component));
    }

    template<typename T>
    T* get(Entity e) {
        return getPool<T>().get(e);
    }

    template<typename T>
    bool has(Entity e) const {
        auto it = m_pools.find(std::type_index(typeid(T)));
        return it != m_pools.end() && it->second->contains(e);
    }

    // Iterate over single component type
    template<typename T, typename Func>
    void forEach(Func&& func) {
        for (auto& [entity, component] : getPool<T>()) {
            func(entity, component);
        }
    }

    // Iterate with component pair
    template<typename T1, typename T2, typename Func>
    void forEach(Func&& func) {
        for (auto& [entity, c1] : getPool<T1>()) {
            auto* c2 = get<T2>(entity);
            if (c2) func(entity, c1, *c2);
        }
    }

    // Iterate with component triple
    template<typename T1, typename T2, typename T3, typename Func>
    void forEach(Func&& func) {
        for (auto& [entity, c1] : getPool<T1>()) {
            auto* c2 = get<T2>(entity);
            auto* c3 = get<T3>(entity);
            if (c2 && c3) func(entity, c1, *c2, *c3);
        }
    }

private:
    template<typename T>
    ComponentPool<T>& getPool() {
        auto type = std::type_index(typeid(T));
        auto it = m_pools.find(type);
        if (it == m_pools.end()) {
            m_pools[type] = std::make_unique<ComponentPool<T>>();
        }
        return static_cast<ComponentPool<T>&>(*m_pools[type]);
    }

    Entity m_nextId = 0;
    std::unordered_set<Entity> m_alive;
    std::unordered_map<std::type_index, std::unique_ptr<IComponentPool>> m_pools;
};
```

#### 3c. Usage After Refactor
```cpp
// Systems use generic forEach
animationSystem.update(registry, dt);
// Inside AnimationSystem:
registry.forEach<Animation, Skeleton>([&](Entity e, Animation& anim, Skeleton& skel) {
    // ...
});

// Can query any combination without adding new methods:
registry.forEach<Transform, RigidBody>([](Entity e, Transform& t, RigidBody& rb) {
    // physics update
});

registry.forEach<Transform, PlayerController, FacingDirection>([](Entity e, ...) {
    // player update
});
```

---

### Priority 4: Basic Event System (Future consideration)

#### 4a. Simple Event Bus
```cpp
// src/ecs/Events.h
#pragma once
#include "Entity.h"
#include <functional>
#include <vector>
#include <any>
#include <typeindex>
#include <unordered_map>

struct AnimationFinished { Entity entity; int clipIndex; };
struct EntityDied { Entity entity; };
struct CollisionEvent { Entity a; Entity b; glm::vec3 point; };

class EventBus {
public:
    template<typename E, typename Func>
    void subscribe(Func&& callback) {
        auto type = std::type_index(typeid(E));
        m_handlers[type].push_back([cb = std::forward<Func>(callback)](const std::any& e) {
            cb(std::any_cast<const E&>(e));
        });
    }

    template<typename E>
    void publish(const E& event) {
        auto type = std::type_index(typeid(E));
        auto it = m_handlers.find(type);
        if (it != m_handlers.end()) {
            for (auto& handler : it->second) {
                handler(event);
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(const std::any&)>>> m_handlers;
};
```

#### 4b. Example Usage
```cpp
// In main.cpp
EventBus events;

events.subscribe<AnimationFinished>([](const AnimationFinished& e) {
    std::cout << "Animation " << e.clipIndex << " finished on entity " << e.entity << std::endl;
});

// In AnimationSystem
if (anim.time >= clip.duration && !anim.loop) {
    events.publish(AnimationFinished{entity, anim.clipIndex});
}
```

---

## Implementation Order

### Phase 1: Quick Wins (Do First)
1. Add `has*()` methods to Registry
2. Add `isAlive()` with `m_alive` set
3. Move animation clips into Animation component
4. Make AnimationSystem stateless

### Phase 2: Decouple Camera and Player
1. Create FacingDirection component
2. Add to Registry (or use generic system if Phase 3 done)
3. Update CameraOrbitSystem
4. Update PlayerMovementSystem (remove cameraYaw parameter)
5. Update FollowCameraSystem
6. Update RenderSystem
7. Update main.cpp

### Phase 3: Generic Components (Optional, for scalability)
1. Create ComponentPool template
2. Refactor Registry to use pools
3. Add generic `add<T>()`, `get<T>()`, `has<T>()`
4. Add generic `forEach<T...>()` methods
5. Migrate existing code to new API
6. Remove old per-component methods

### Phase 4: Events (Future)
1. Create EventBus
2. Define event types
3. Wire into systems
4. Useful when adding: AI, multiplayer, achievements, sound triggers

---

## File Changes Summary

### New Files
- `src/ecs/components/FacingDirection.h`
- `src/ecs/ComponentPool.h` (optional, Phase 3)
- `src/ecs/Events.h` (optional, Phase 4)

### Modified Files
- `src/ecs/Registry.h` - Add has*/isAlive, or full refactor
- `src/ecs/components/Animation.h` - Add `clips` vector
- `src/ecs/components/FollowTarget.h` - Remove `yaw`
- `src/ecs/systems/AnimationSystem.h` - Remove m_clips, use component
- `src/ecs/systems/PlayerMovementSystem.h` - Use FacingDirection
- `src/ecs/systems/CameraOrbitSystem.h` - Write to FacingDirection
- `src/ecs/systems/FollowCameraSystem.h` - Read FacingDirection from target
- `src/ecs/systems/RenderSystem.h` - Read FacingDirection from target
- `main.cpp` - Add FacingDirection, update wiring

---

## Current File Structure Reference

```
src/ecs/
├── Entity.h              # Entity = uint32_t, NULL_ENTITY = 0
├── Registry.h            # Component storage, forEachXxx() methods
│
├── components/
│   ├── Animation.h       # clipIndex, time, playing (clips in AnimationSystem)
│   ├── Camera.h          # fov, near, far, active
│   ├── Collider.h        # GroundPlane, BoxCollider
│   ├── FollowTarget.h    # target, distance, height, shoulderOffset, yaw, pitch
│   ├── Mesh.h            # MeshData, MeshGroup
│   ├── PlayerController.h# moveSpeed, turnSpeed
│   ├── Renderable.h      # ShaderType
│   ├── RigidBody.h       # velocity, gravity, grounded
│   ├── Skeleton.h        # joints, boneMatrices, bindPoseTransforms
│   └── Transform.h       # position, rotation, scale
│
└── systems/
    ├── AnimationSystem.h     # m_clips map (should move to component)
    ├── CameraOrbitSystem.h   # mouseX/Y → FollowTarget.yaw/pitch
    ├── CollisionSystem.h     # RigidBody vs BoxCollider
    ├── FollowCameraSystem.h  # Position camera behind target
    ├── InputSystem.h         # SDL polling, mouse capture
    ├── PhysicsSystem.h       # Gravity, velocity integration
    ├── PlayerMovementSystem.h# WASD + animation selection
    ├── RenderSystem.h        # Draw all renderables
    └── SkeletonSystem.h      # Compute bone matrices
```

---

## Key Code Snippets for Reference

### Current Animation Component (src/ecs/components/Animation.h)
```cpp
struct Animation {
    int clipIndex = 0;
    float time = 0.0f;
    bool playing = false;
};
```

### Current FollowTarget (src/ecs/components/FollowTarget.h)
```cpp
struct FollowTarget {
    Entity target = NULL_ENTITY;
    float distance = 2.2f;
    float height = 1.2f;
    float shoulderOffset = 2.4f;
    float lookAhead = 5.0f;
    float yaw = 0.0f;           // Controls character + camera
    float pitch = 10.0f;
    float sensitivity = 0.3f;
};
```

### PlayerMovementSystem Signature (current)
```cpp
void update(Registry& registry, float dt, float cameraYaw)
```

### PlayerMovementSystem Signature (after Phase 2)
```cpp
void update(Registry& registry, float dt)  // No more cameraYaw param
```

---

## Notes for Implementation

1. **Phase 1 is safest** - No breaking changes, just additions
2. **Phase 2 is cleanest win** - Decouples systems properly
3. **Phase 3 requires careful migration** - All systems need updating
4. **Keep backward compat during migration** - Add new API alongside old
5. **Test after each phase** - Movement, animation, camera should all work

---

## Current Working Features (Don't Break These)

- WASD moves character relative to camera facing
- Mouse rotates facing direction (character + camera follow)
- Character always faces movement direction with smooth rotation
- Camera positioned behind-right (over-shoulder view)
- Walking animation (forward, no shift) - clip 1
- Running animation (forward + shift) - clip 0
- Backrun animation (backward) - clip 2
- Bind pose reset when stopped
- Speed: backward 0.25x, walking 0.5x, sprinting 1.0x
