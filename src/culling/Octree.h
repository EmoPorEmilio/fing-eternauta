#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include "Frustum.h"

// Octree for O(log n) frustum culling of static objects
template<typename T>
class Octree {
public:
    static constexpr int MAX_OBJECTS_PER_NODE = 8;
    static constexpr int MAX_DEPTH = 8;

    struct Node {
        AABB bounds;
        std::vector<const T*> objects;
        std::array<std::unique_ptr<Node>, 8> children;
        bool isLeaf = true;
        Node(const AABB& b) : bounds(b) {}
    };

    Octree() = default;

    void build(const std::vector<T>& objects, std::function<AABB(const T&)> getAABB) {
        if (objects.empty()) return;

        m_getAABB = getAABB;

        AABB worldBounds = getAABB(objects[0]);
        for (size_t i = 1; i < objects.size(); ++i) {
            AABB objBounds = getAABB(objects[i]);
            worldBounds.min = glm::min(worldBounds.min, objBounds.min);
            worldBounds.max = glm::max(worldBounds.max, objBounds.max);
        }

        glm::vec3 padding(1.0f);
        worldBounds.min -= padding;
        worldBounds.max += padding;

        // Make cubic for proper subdivision
        glm::vec3 size = worldBounds.max - worldBounds.min;
        float maxSize = glm::max(glm::max(size.x, size.y), size.z);
        glm::vec3 center = worldBounds.getCenter();
        worldBounds.min = center - glm::vec3(maxSize * 0.5f);
        worldBounds.max = center + glm::vec3(maxSize * 0.5f);

        m_root = std::make_unique<Node>(worldBounds);

        for (const auto& obj : objects) {
            insert(m_root.get(), &obj, 0);
        }
    }

    void queryFrustum(const Frustum& frustum, std::function<void(const T&)> callback) const {
        if (!m_root) return;
        queryNode(m_root.get(), frustum, callback);
    }

    void queryRadius(const glm::vec3& center, float radius, std::function<void(const T&)> callback) const {
        if (!m_root) return;
        AABB queryBox = AABB::fromCenterExtents(center, glm::vec3(radius));
        queryRadiusNode(m_root.get(), center, radius * radius, queryBox, callback);
    }

    bool raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDist, float& hitDist) const {
        if (!m_root) return false;

        glm::vec3 dirInv(
            direction.x != 0.0f ? 1.0f / direction.x : 1e30f,
            direction.y != 0.0f ? 1.0f / direction.y : 1e30f,
            direction.z != 0.0f ? 1.0f / direction.z : 1e30f
        );

        float closestHit = maxDist;
        raycastNode(m_root.get(), origin, dirInv, closestHit);

        if (closestHit < maxDist) {
            hitDist = closestHit;
            return true;
        }
        return false;
    }

    struct Stats {
        size_t totalNodes = 0;
        size_t leafNodes = 0;
        size_t totalObjects = 0;
        size_t maxDepth = 0;
    };

    Stats getStats() const {
        Stats stats;
        if (m_root) {
            collectStats(m_root.get(), 0, stats);
        }
        return stats;
    }

private:
    std::unique_ptr<Node> m_root;
    std::function<AABB(const T&)> m_getAABB;

    void insert(Node* node, const T* object, int depth) {
        AABB objBounds = m_getAABB(*object);

        if (node->isLeaf) {
            node->objects.push_back(object);

            if (node->objects.size() > MAX_OBJECTS_PER_NODE && depth < MAX_DEPTH) {
                subdivide(node);
                std::vector<const T*> toRedistribute = std::move(node->objects);
                node->objects.clear();
                for (const T* obj : toRedistribute) {
                    insertIntoChildren(node, obj, depth);
                }
            }
            return;
        }

        insertIntoChildren(node, object, depth);
    }

    void insertIntoChildren(Node* node, const T* object, int depth) {
        AABB objBounds = m_getAABB(*object);
        bool inserted = false;

        for (auto& child : node->children) {
            if (child && child->bounds.intersects(objBounds)) {
                insert(child.get(), object, depth + 1);
                inserted = true;
            }
        }

        if (!inserted) {
            node->objects.push_back(object);
        }
    }

    void subdivide(Node* node) {
        node->isLeaf = false;
        glm::vec3 center = node->bounds.getCenter();
        glm::vec3 halfSize = node->bounds.getExtents() * 0.5f;

        for (int i = 0; i < 8; ++i) {
            glm::vec3 childCenter = center;
            childCenter.x += (i & 1) ? halfSize.x : -halfSize.x;
            childCenter.y += (i & 2) ? halfSize.y : -halfSize.y;
            childCenter.z += (i & 4) ? halfSize.z : -halfSize.z;

            AABB childBounds = AABB::fromCenterExtents(childCenter, halfSize);
            node->children[i] = std::make_unique<Node>(childBounds);
        }
    }

    void queryNode(const Node* node, const Frustum& frustum,
                   std::function<void(const T&)>& callback) const {
        if (frustum.isBoxOutside(node->bounds)) return;

        for (const T* obj : node->objects) {
            AABB objBounds = m_getAABB(*obj);
            if (frustum.isBoxVisible(objBounds)) {
                callback(*obj);
            }
        }

        if (!node->isLeaf) {
            for (const auto& child : node->children) {
                if (child) queryNode(child.get(), frustum, callback);
            }
        }
    }

    void queryRadiusNode(const Node* node, const glm::vec3& center,
                         float radiusSq, const AABB& queryBox,
                         std::function<void(const T&)>& callback) const {
        if (!node->bounds.intersects(queryBox)) return;

        for (const T* obj : node->objects) {
            AABB objBounds = m_getAABB(*obj);
            if (objBounds.intersects(queryBox)) {
                callback(*obj);
            }
        }

        if (!node->isLeaf) {
            for (const auto& child : node->children) {
                if (child) queryRadiusNode(child.get(), center, radiusSq, queryBox, callback);
            }
        }
    }

    void collectStats(const Node* node, size_t depth, Stats& stats) const {
        stats.totalNodes++;
        stats.totalObjects += node->objects.size();
        stats.maxDepth = glm::max(stats.maxDepth, depth);

        if (node->isLeaf) {
            stats.leafNodes++;
        } else {
            for (const auto& child : node->children) {
                if (child) {
                    collectStats(child.get(), depth + 1, stats);
                }
            }
        }
    }

    void raycastNode(const Node* node, const glm::vec3& origin,
                     const glm::vec3& dirInv, float& closestHit) const {
        float nodeDist;
        bool hitsNode = node->bounds.raycast(origin, dirInv, 1e10f, nodeDist);
        if (!hitsNode && !node->bounds.contains(origin)) return;

        for (const T* obj : node->objects) {
            AABB objBounds = m_getAABB(*obj);
            float objDist;
            if (objBounds.raycast(origin, dirInv, closestHit, objDist)) {
                if (objDist < closestHit && objDist >= 0.0f) {
                    closestHit = objDist;
                }
            }
        }

        if (!node->isLeaf) {
            for (const auto& child : node->children) {
                if (child) raycastNode(child.get(), origin, dirInv, closestHit);
            }
        }
    }
};
