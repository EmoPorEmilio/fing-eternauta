#pragma once

#include <glm/glm.hpp>
#include <array>

// Axis-Aligned Bounding Box for spatial queries
struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(0.0f), max(0.0f) {}
    AABB(const glm::vec3& min, const glm::vec3& max) : min(min), max(max) {}

    // Create AABB from center and half-extents
    static AABB fromCenterExtents(const glm::vec3& center, const glm::vec3& halfExtents) {
        return AABB(center - halfExtents, center + halfExtents);
    }

    glm::vec3 getCenter() const { return (min + max) * 0.5f; }
    glm::vec3 getExtents() const { return (max - min) * 0.5f; }

    bool contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }

    bool intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }

    // Ray-AABB intersection using slab method
    // Returns true if ray hits box, sets tMin to distance along ray
    // origin: ray start point
    // dirInv: 1.0 / ray direction (precomputed for efficiency)
    // maxDist: maximum distance to check
    bool raycast(const glm::vec3& origin, const glm::vec3& dirInv, float maxDist, float& tMin) const {
        float t1 = (min.x - origin.x) * dirInv.x;
        float t2 = (max.x - origin.x) * dirInv.x;
        float t3 = (min.y - origin.y) * dirInv.y;
        float t4 = (max.y - origin.y) * dirInv.y;
        float t5 = (min.z - origin.z) * dirInv.z;
        float t6 = (max.z - origin.z) * dirInv.z;

        float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
        float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

        // Box is behind ray or ray misses box
        if (tmax < 0 || tmin > tmax) {
            return false;
        }

        // Hit is beyond max distance
        if (tmin > maxDist) {
            return false;
        }

        // If tmin < 0, ray starts inside box, use tmax
        tMin = (tmin < 0) ? tmax : tmin;
        return tMin <= maxDist;
    }
};

// Frustum plane representation (ax + by + cz + d = 0)
struct Plane {
    glm::vec3 normal;
    float distance;

    Plane() : normal(0.0f, 1.0f, 0.0f), distance(0.0f) {}
    Plane(const glm::vec3& n, float d) : normal(n), distance(d) {}

    // Signed distance from point to plane (positive = front, negative = behind)
    float distanceToPoint(const glm::vec3& point) const {
        return glm::dot(normal, point) + distance;
    }

    void normalize() {
        float len = glm::length(normal);
        if (len > 0.0001f) {
            normal /= len;
            distance /= len;
        }
    }
};

// Camera view frustum for culling
// Extracts 6 planes from view-projection matrix
class Frustum {
public:
    enum PlaneIndex {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR,
        PLANE_COUNT
    };

    Frustum() = default;

    // Extract frustum planes from combined view-projection matrix
    // Uses Gribb-Hartmann method
    void extractFromMatrix(const glm::mat4& viewProjection) {
        // Left plane
        m_planes[LEFT].normal.x = viewProjection[0][3] + viewProjection[0][0];
        m_planes[LEFT].normal.y = viewProjection[1][3] + viewProjection[1][0];
        m_planes[LEFT].normal.z = viewProjection[2][3] + viewProjection[2][0];
        m_planes[LEFT].distance = viewProjection[3][3] + viewProjection[3][0];

        // Right plane
        m_planes[RIGHT].normal.x = viewProjection[0][3] - viewProjection[0][0];
        m_planes[RIGHT].normal.y = viewProjection[1][3] - viewProjection[1][0];
        m_planes[RIGHT].normal.z = viewProjection[2][3] - viewProjection[2][0];
        m_planes[RIGHT].distance = viewProjection[3][3] - viewProjection[3][0];

        // Bottom plane
        m_planes[BOTTOM].normal.x = viewProjection[0][3] + viewProjection[0][1];
        m_planes[BOTTOM].normal.y = viewProjection[1][3] + viewProjection[1][1];
        m_planes[BOTTOM].normal.z = viewProjection[2][3] + viewProjection[2][1];
        m_planes[BOTTOM].distance = viewProjection[3][3] + viewProjection[3][1];

        // Top plane
        m_planes[TOP].normal.x = viewProjection[0][3] - viewProjection[0][1];
        m_planes[TOP].normal.y = viewProjection[1][3] - viewProjection[1][1];
        m_planes[TOP].normal.z = viewProjection[2][3] - viewProjection[2][1];
        m_planes[TOP].distance = viewProjection[3][3] - viewProjection[3][1];

        // Near plane
        m_planes[NEAR].normal.x = viewProjection[0][3] + viewProjection[0][2];
        m_planes[NEAR].normal.y = viewProjection[1][3] + viewProjection[1][2];
        m_planes[NEAR].normal.z = viewProjection[2][3] + viewProjection[2][2];
        m_planes[NEAR].distance = viewProjection[3][3] + viewProjection[3][2];

        // Far plane
        m_planes[FAR].normal.x = viewProjection[0][3] - viewProjection[0][2];
        m_planes[FAR].normal.y = viewProjection[1][3] - viewProjection[1][2];
        m_planes[FAR].normal.z = viewProjection[2][3] - viewProjection[2][2];
        m_planes[FAR].distance = viewProjection[3][3] - viewProjection[3][2];

        // Normalize all planes
        for (auto& plane : m_planes) {
            plane.normalize();
        }
    }

    // Test if AABB is completely outside frustum
    // Returns true if AABB should be culled (not visible)
    bool isBoxOutside(const AABB& box) const {
        for (const auto& plane : m_planes) {
            // Find the corner most aligned with plane normal (p-vertex)
            glm::vec3 pVertex;
            pVertex.x = (plane.normal.x >= 0) ? box.max.x : box.min.x;
            pVertex.y = (plane.normal.y >= 0) ? box.max.y : box.min.y;
            pVertex.z = (plane.normal.z >= 0) ? box.max.z : box.min.z;

            // If p-vertex is behind plane, box is completely outside
            if (plane.distanceToPoint(pVertex) < 0) {
                return true;
            }
        }
        return false;
    }

    // Test if AABB intersects or is inside frustum
    bool isBoxVisible(const AABB& box) const {
        return !isBoxOutside(box);
    }

    // Test if sphere is visible (useful for quick checks)
    bool isSphereVisible(const glm::vec3& center, float radius) const {
        for (const auto& plane : m_planes) {
            if (plane.distanceToPoint(center) < -radius) {
                return false;
            }
        }
        return true;
    }

    const Plane& getPlane(PlaneIndex index) const { return m_planes[index]; }

private:
    std::array<Plane, PLANE_COUNT> m_planes;
};
