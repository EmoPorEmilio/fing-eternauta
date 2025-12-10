#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include "Frustum.h"

// Octree for spatial partitioning of static objects (buildings)
// Provides O(log n) frustum culling instead of O(n) linear scan
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

    // Build octree from list of objects
    // getAABB: function to extract AABB from object
    void build(const std::vector<T>& objects,
               std::function<AABB(const T&)> getAABB) {
        if (objects.empty()) return;

        m_getAABB = getAABB;

        // Calculate world bounds encompassing all objects
        AABB worldBounds = getAABB(objects[0]);
        for (size_t i = 1; i < objects.size(); ++i) {
            AABB objBounds = getAABB(objects[i]);
            worldBounds.min = glm::min(worldBounds.min, objBounds.min);
            worldBounds.max = glm::max(worldBounds.max, objBounds.max);
        }

        // Expand bounds slightly to ensure all objects fit
        glm::vec3 padding(1.0f);
        worldBounds.min -= padding;
        worldBounds.max += padding;

        // Make bounds cubic for proper octree subdivision
        glm::vec3 size = worldBounds.max - worldBounds.min;
        float maxSize = glm::max(glm::max(size.x, size.y), size.z);
        glm::vec3 center = worldBounds.getCenter();
        worldBounds.min = center - glm::vec3(maxSize * 0.5f);
        worldBounds.max = center + glm::vec3(maxSize * 0.5f);

        m_root = std::make_unique<Node>(worldBounds);

        // Insert all objects
        for (const auto& obj : objects) {
            insert(m_root.get(), &obj, 0);
        }
    }

    // Query all objects whose AABB is visible in the frustum
    // Callback receives each visible object
    void queryFrustum(const Frustum& frustum,
                      std::function<void(const T&)> callback) const {
        if (!m_root) return;
        queryNode(m_root.get(), frustum, callback);
    }

    // Query objects within a distance from a point
    void queryRadius(const glm::vec3& center, float radius,
                     std::function<void(const T&)> callback) const {
        if (!m_root) return;
        AABB queryBox = AABB::fromCenterExtents(center, glm::vec3(radius));
        queryRadiusNode(m_root.get(), center, radius * radius, queryBox, callback);
    }

    // Raycast through octree, find closest hit
    // Returns true if hit found, sets hitDist to distance along ray
    bool raycast(const glm::vec3& origin, const glm::vec3& direction,
                 float maxDist, float& hitDist) const {
        if (!m_root) return false;

        // Precompute inverse direction for slab method
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

    // Get statistics for debugging
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

        // If leaf and not at max capacity/depth, add to this node
        if (node->isLeaf) {
            node->objects.push_back(object);

            // Subdivide if over capacity and not at max depth
            if (node->objects.size() > MAX_OBJECTS_PER_NODE && depth < MAX_DEPTH) {
                subdivide(node);

                // Redistribute objects to children
                std::vector<const T*> toRedistribute = std::move(node->objects);
                node->objects.clear();

                for (const T* obj : toRedistribute) {
                    insertIntoChildren(node, obj, depth);
                }
            }
            return;
        }

        // Not a leaf - insert into appropriate children
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

        // If object spans multiple children or doesn't fit, keep in parent
        if (!inserted) {
            node->objects.push_back(object);
        }
    }

    void subdivide(Node* node) {
        node->isLeaf = false;

        glm::vec3 center = node->bounds.getCenter();
        glm::vec3 halfSize = node->bounds.getExtents() * 0.5f;

        // Create 8 child nodes
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
        // Early out if node bounds are outside frustum
        if (frustum.isBoxOutside(node->bounds)) {
            return;
        }

        // Check objects in this node
        for (const T* obj : node->objects) {
            AABB objBounds = m_getAABB(*obj);
            if (frustum.isBoxVisible(objBounds)) {
                callback(*obj);
            }
        }

        // Recurse into children
        if (!node->isLeaf) {
            for (const auto& child : node->children) {
                if (child) {
                    queryNode(child.get(), frustum, callback);
                }
            }
        }
    }

    void queryRadiusNode(const Node* node, const glm::vec3& center,
                         float radiusSq, const AABB& queryBox,
                         std::function<void(const T&)>& callback) const {
        // Early out if node doesn't intersect query box
        if (!node->bounds.intersects(queryBox)) {
            return;
        }

        // Check objects in this node - use AABB intersection for collision detection
        for (const T* obj : node->objects) {
            AABB objBounds = m_getAABB(*obj);
            // Check if object AABB intersects query box (more accurate for collision)
            if (objBounds.intersects(queryBox)) {
                callback(*obj);
            }
        }

        // Recurse into children
        if (!node->isLeaf) {
            for (const auto& child : node->children) {
                if (child) {
                    queryRadiusNode(child.get(), center, radiusSq, queryBox, callback);
                }
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
        // Check if ray intersects this node's bounds at all
        // We need to handle the case where ray origin is inside the node
        float nodeDist;
        bool hitsNode = node->bounds.raycast(origin, dirInv, 1e10f, nodeDist);

        // If ray doesn't hit node AND origin is not inside node, skip
        if (!hitsNode && !node->bounds.contains(origin)) {
            return;
        }

        // Check objects in this node
        for (const T* obj : node->objects) {
            AABB objBounds = m_getAABB(*obj);
            float objDist;
            if (objBounds.raycast(origin, dirInv, closestHit, objDist)) {
                if (objDist < closestHit && objDist >= 0.0f) {
                    closestHit = objDist;
                }
            }
        }

        // Recurse into children
        if (!node->isLeaf) {
            for (const auto& child : node->children) {
                if (child) {
                    raycastNode(child.get(), origin, dirInv, closestHit);
                }
            }
        }
    }
};
