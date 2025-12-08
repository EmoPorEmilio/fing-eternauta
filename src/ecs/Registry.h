#pragma once
#include "Entity.h"
#include "components/Transform.h"
#include "components/Mesh.h"
#include "components/Skeleton.h"
#include "components/Animation.h"
#include "components/Renderable.h"
#include "components/Camera.h"
#include "components/RigidBody.h"
#include "components/Collider.h"
#include "components/PlayerController.h"
#include "components/FollowTarget.h"
#include "components/FacingDirection.h"
#include "components/UIText.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

class Registry {
public:
    Entity create() {
        Entity e = ++m_nextId;
        m_alive.insert(e);
        return e;
    }

    void destroy(Entity e) {
        m_alive.erase(e);
        m_transforms.erase(e);
        m_meshGroups.erase(e);
        m_skeletons.erase(e);
        m_animations.erase(e);
        m_renderables.erase(e);
        m_cameras.erase(e);
        m_rigidBodies.erase(e);
        m_groundPlanes.erase(e);
        m_boxColliders.erase(e);
        m_playerControllers.erase(e);
        m_followTargets.erase(e);
        m_facingDirections.erase(e);
        m_uiTexts.erase(e);
    }

    bool isAlive(Entity e) const {
        return m_alive.count(e) > 0;
    }

    // has*() component checks
    bool hasTransform(Entity e) const { return m_transforms.count(e) > 0; }
    bool hasMeshGroup(Entity e) const { return m_meshGroups.count(e) > 0; }
    bool hasSkeleton(Entity e) const { return m_skeletons.count(e) > 0; }
    bool hasAnimation(Entity e) const { return m_animations.count(e) > 0; }
    bool hasRenderable(Entity e) const { return m_renderables.count(e) > 0; }
    bool hasCamera(Entity e) const { return m_cameras.count(e) > 0; }
    bool hasRigidBody(Entity e) const { return m_rigidBodies.count(e) > 0; }
    bool hasGroundPlane(Entity e) const { return m_groundPlanes.count(e) > 0; }
    bool hasBoxCollider(Entity e) const { return m_boxColliders.count(e) > 0; }
    bool hasPlayerController(Entity e) const { return m_playerControllers.count(e) > 0; }
    bool hasFollowTarget(Entity e) const { return m_followTargets.count(e) > 0; }
    bool hasFacingDirection(Entity e) const { return m_facingDirections.count(e) > 0; }
    bool hasUIText(Entity e) const { return m_uiTexts.count(e) > 0; }

    // Transform
    Transform& addTransform(Entity e, Transform t = {}) {
        m_transforms[e] = t;
        return m_transforms[e];
    }
    Transform* getTransform(Entity e) {
        auto it = m_transforms.find(e);
        return it != m_transforms.end() ? &it->second : nullptr;
    }

    // MeshGroup
    MeshGroup& addMeshGroup(Entity e, MeshGroup m = {}) {
        m_meshGroups[e] = std::move(m);
        return m_meshGroups[e];
    }
    MeshGroup* getMeshGroup(Entity e) {
        auto it = m_meshGroups.find(e);
        return it != m_meshGroups.end() ? &it->second : nullptr;
    }

    // Skeleton
    Skeleton& addSkeleton(Entity e, Skeleton s = {}) {
        m_skeletons[e] = std::move(s);
        return m_skeletons[e];
    }
    Skeleton* getSkeleton(Entity e) {
        auto it = m_skeletons.find(e);
        return it != m_skeletons.end() ? &it->second : nullptr;
    }

    // Animation
    Animation& addAnimation(Entity e, Animation a = {}) {
        m_animations[e] = a;
        return m_animations[e];
    }
    Animation* getAnimation(Entity e) {
        auto it = m_animations.find(e);
        return it != m_animations.end() ? &it->second : nullptr;
    }

    // Renderable
    Renderable& addRenderable(Entity e, Renderable r = {}) {
        m_renderables[e] = r;
        return m_renderables[e];
    }
    Renderable* getRenderable(Entity e) {
        auto it = m_renderables.find(e);
        return it != m_renderables.end() ? &it->second : nullptr;
    }

    // Camera
    CameraComponent& addCamera(Entity e, CameraComponent c = {}) {
        m_cameras[e] = c;
        return m_cameras[e];
    }
    CameraComponent* getCamera(Entity e) {
        auto it = m_cameras.find(e);
        return it != m_cameras.end() ? &it->second : nullptr;
    }

    // RigidBody
    RigidBody& addRigidBody(Entity e, RigidBody rb = {}) {
        m_rigidBodies[e] = rb;
        return m_rigidBodies[e];
    }
    RigidBody* getRigidBody(Entity e) {
        auto it = m_rigidBodies.find(e);
        return it != m_rigidBodies.end() ? &it->second : nullptr;
    }

    // GroundPlane
    GroundPlane& addGroundPlane(Entity e, GroundPlane g = {}) {
        m_groundPlanes[e] = g;
        return m_groundPlanes[e];
    }
    GroundPlane* getGroundPlane(Entity e) {
        auto it = m_groundPlanes.find(e);
        return it != m_groundPlanes.end() ? &it->second : nullptr;
    }

    // BoxCollider
    BoxCollider& addBoxCollider(Entity e, BoxCollider b = {}) {
        m_boxColliders[e] = b;
        return m_boxColliders[e];
    }
    BoxCollider* getBoxCollider(Entity e) {
        auto it = m_boxColliders.find(e);
        return it != m_boxColliders.end() ? &it->second : nullptr;
    }

    // PlayerController
    PlayerController& addPlayerController(Entity e, PlayerController pc = {}) {
        m_playerControllers[e] = pc;
        return m_playerControllers[e];
    }
    PlayerController* getPlayerController(Entity e) {
        auto it = m_playerControllers.find(e);
        return it != m_playerControllers.end() ? &it->second : nullptr;
    }

    // FollowTarget
    FollowTarget& addFollowTarget(Entity e, FollowTarget ft = {}) {
        m_followTargets[e] = ft;
        return m_followTargets[e];
    }
    FollowTarget* getFollowTarget(Entity e) {
        auto it = m_followTargets.find(e);
        return it != m_followTargets.end() ? &it->second : nullptr;
    }

    // FacingDirection
    FacingDirection& addFacingDirection(Entity e, FacingDirection fd = {}) {
        m_facingDirections[e] = fd;
        return m_facingDirections[e];
    }
    FacingDirection* getFacingDirection(Entity e) {
        auto it = m_facingDirections.find(e);
        return it != m_facingDirections.end() ? &it->second : nullptr;
    }

    // UIText
    UIText& addUIText(Entity e, UIText ut = {}) {
        m_uiTexts[e] = std::move(ut);
        return m_uiTexts[e];
    }
    UIText* getUIText(Entity e) {
        auto it = m_uiTexts.find(e);
        return it != m_uiTexts.end() ? &it->second : nullptr;
    }

    // Iteration helpers
    template<typename Func>
    void forEachRenderable(Func&& func) {
        for (auto& [entity, renderable] : m_renderables) {
            auto* transform = getTransform(entity);
            auto* meshGroup = getMeshGroup(entity);
            if (transform && meshGroup) {
                func(entity, *transform, *meshGroup, renderable);
            }
        }
    }

    template<typename Func>
    void forEachAnimated(Func&& func) {
        for (auto& [entity, animation] : m_animations) {
            auto* skeleton = getSkeleton(entity);
            if (skeleton) {
                func(entity, animation, *skeleton);
            }
        }
    }

    template<typename Func>
    void forEachSkeleton(Func&& func) {
        for (auto& [entity, skeleton] : m_skeletons) {
            func(entity, skeleton);
        }
    }

    template<typename Func>
    void forEachCamera(Func&& func) {
        for (auto& [entity, camera] : m_cameras) {
            auto* transform = getTransform(entity);
            if (transform) {
                func(entity, *transform, camera);
            }
        }
    }

    Entity getActiveCamera() {
        for (auto& [entity, camera] : m_cameras) {
            if (camera.active) return entity;
        }
        return NULL_ENTITY;
    }

    template<typename Func>
    void forEachRigidBody(Func&& func) {
        for (auto& [entity, rigidBody] : m_rigidBodies) {
            auto* transform = getTransform(entity);
            if (transform) {
                func(entity, *transform, rigidBody);
            }
        }
    }

    template<typename Func>
    void forEachGroundPlane(Func&& func) {
        for (auto& [entity, ground] : m_groundPlanes) {
            func(entity, ground);
        }
    }

    template<typename Func>
    void forEachBoxCollider(Func&& func) {
        for (auto& [entity, box] : m_boxColliders) {
            auto* transform = getTransform(entity);
            if (transform) {
                func(entity, *transform, box);
            }
        }
    }

    template<typename Func>
    void forEachPlayerController(Func&& func) {
        for (auto& [entity, pc] : m_playerControllers) {
            auto* transform = getTransform(entity);
            if (transform) {
                func(entity, *transform, pc);
            }
        }
    }

    template<typename Func>
    void forEachFollowTarget(Func&& func) {
        for (auto& [entity, ft] : m_followTargets) {
            auto* transform = getTransform(entity);
            if (transform) {
                func(entity, *transform, ft);
            }
        }
    }

    template<typename Func>
    void forEachFacingDirection(Func&& func) {
        for (auto& [entity, fd] : m_facingDirections) {
            auto* transform = getTransform(entity);
            if (transform) {
                func(entity, *transform, fd);
            }
        }
    }

    template<typename Func>
    void forEachUIText(Func&& func) {
        for (auto& [entity, uiText] : m_uiTexts) {
            func(entity, uiText);
        }
    }

private:
    Entity m_nextId = 0;
    std::unordered_set<Entity> m_alive;
    std::unordered_map<Entity, Transform> m_transforms;
    std::unordered_map<Entity, MeshGroup> m_meshGroups;
    std::unordered_map<Entity, Skeleton> m_skeletons;
    std::unordered_map<Entity, Animation> m_animations;
    std::unordered_map<Entity, Renderable> m_renderables;
    std::unordered_map<Entity, CameraComponent> m_cameras;
    std::unordered_map<Entity, RigidBody> m_rigidBodies;
    std::unordered_map<Entity, GroundPlane> m_groundPlanes;
    std::unordered_map<Entity, BoxCollider> m_boxColliders;
    std::unordered_map<Entity, PlayerController> m_playerControllers;
    std::unordered_map<Entity, FollowTarget> m_followTargets;
    std::unordered_map<Entity, FacingDirection> m_facingDirections;
    std::unordered_map<Entity, UIText> m_uiTexts;
};
