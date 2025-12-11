#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include "Frustum.h"
#include "Octree.h"
#include "../procedural/BuildingGenerator.h"
#include "../rendering/InstancedRenderer.h"

// Render parameters for building main pass
struct BuildingRenderParams {
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 lightSpaceMatrix;
    glm::vec3 lightDir;
    glm::vec3 viewPos;
    GLuint texture = 0;
    GLuint normalMap = 0;
    GLuint shadowMap = 0;
    float textureScale = 4.0f;
    bool fogEnabled = false;
    bool shadowsEnabled = true;
};

// Orchestrates building visibility culling using octree + frustum
// Provides culled building list to InstancedRenderer for efficient drawing
class BuildingCuller {
public:
    BuildingCuller() = default;

    // Initialize with building data - builds octree
    void init(const std::vector<BuildingGenerator::BuildingData>& buildings,
              size_t maxVisibleBuildings) {
        m_buildings = &buildings;

        // Build octree with building AABBs
        m_octree.build(buildings, [](const BuildingGenerator::BuildingData& b) -> AABB {
            glm::vec3 halfExtents(b.width * 0.5f, b.height * 0.5f, b.depth * 0.5f);
            glm::vec3 center = b.position + glm::vec3(0.0f, b.height * 0.5f, 0.0f);
            return AABB::fromCenterExtents(center, halfExtents);
        });

        // Initialize instanced renderers (one for camera view, one for shadow pass)
        m_instancedRenderer.init(maxVisibleBuildings);
        m_shadowInstancedRenderer.init(maxVisibleBuildings);

        // Print octree stats
        auto stats = m_octree.getStats();
        std::cout << "BuildingCuller octree built: "
                  << stats.totalNodes << " nodes, "
                  << stats.leafNodes << " leaves, "
                  << stats.totalObjects << " objects stored, "
                  << "max depth " << stats.maxDepth << std::endl;
    }

    // Update visibility based on camera frustum
    // Call once per frame before rendering
    void update(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, float maxRenderDistance) {
        m_visibleCount = 0;
        m_instancedRenderer.beginFrame();

        // Extract frustum from view-projection
        glm::mat4 viewProj = projection * view;
        m_frustum.extractFromMatrix(viewProj);

        // Distance culling radius squared
        float maxDistSq = maxRenderDistance * maxRenderDistance;

        // Query octree for visible buildings
        m_octree.queryFrustum(m_frustum, [&](const BuildingGenerator::BuildingData& building) {
            // Additional distance check
            glm::vec3 toBuilding = building.position - cameraPos;
            float distSq = glm::dot(toBuilding, toBuilding);

            if (distSq <= maxDistSq) {
                m_instancedRenderer.addInstance(building.position,
                    glm::vec3(building.width, building.height, building.depth));
                m_visibleCount++;
            }
        });
    }

    // Render all visible buildings (main pass) with full material setup
    void render(const Mesh& buildingMesh, Shader& shader, const BuildingRenderParams& params) {
        if (m_instancedRenderer.getInstanceCount() == 0) return;

        shader.use();

        // Set view/projection uniforms
        shader.setMat4("uView", params.view);
        shader.setMat4("uProjection", params.projection);
        shader.setMat4("uLightSpaceMatrix", params.lightSpaceMatrix);
        shader.setVec3("uLightDir", params.lightDir);
        shader.setVec3("uViewPos", params.viewPos);

        // Material settings
        shader.setInt("uTriplanarMapping", 1);  // Buildings use triplanar
        shader.setFloat("uTextureScale", params.textureScale);
        shader.setInt("uFogEnabled", params.fogEnabled ? 1 : 0);
        shader.setInt("uShadowsEnabled", params.shadowsEnabled ? 1 : 0);

        // Bind textures
        if (params.texture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, params.texture);
            shader.setInt("uTexture", 0);
            shader.setInt("uHasTexture", 1);
        } else {
            shader.setInt("uHasTexture", 0);
        }

        if (params.normalMap) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, params.normalMap);
            shader.setInt("uNormalMap", 1);
            shader.setInt("uHasNormalMap", 1);
        } else {
            shader.setInt("uHasNormalMap", 0);
        }

        if (params.shadowMap) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, params.shadowMap);
            shader.setInt("uShadowMap", 2);
        }

        // Do the instanced draw
        m_instancedRenderer.render(buildingMesh, shader);
    }

    // Update shadow caster visibility - includes all buildings within shadow distance
    // This should be called before renderShadows() to populate shadow instances
    // Uses radius query (not camera frustum) so buildings behind camera still cast shadows
    void updateShadowCasters(const glm::mat4& /*lightSpaceMatrix*/, const glm::vec3& cameraPos, float shadowDistance) {
        m_shadowInstancedRenderer.beginFrame();
        m_shadowVisibleCount = 0;

        // Query all buildings within shadow distance using radius query
        // This ensures buildings behind the camera can still cast shadows into the visible area
        m_octree.queryRadius(cameraPos, shadowDistance, [&](const BuildingGenerator::BuildingData& building) {
            m_shadowInstancedRenderer.addInstance(building.position,
                glm::vec3(building.width, building.height, building.depth));
            m_shadowVisibleCount++;
        });
    }

    // Render shadow pass for shadow casters
    void renderShadows(const Mesh& buildingMesh, Shader& depthShader,
                       const glm::mat4& lightSpaceMatrix) {
        m_shadowInstancedRenderer.renderShadow(buildingMesh, depthShader, lightSpaceMatrix);
    }

    size_t getShadowCasterCount() const { return m_shadowVisibleCount; }

    size_t getVisibleCount() const { return m_visibleCount; }
    size_t getTotalCount() const { return m_buildings ? m_buildings->size() : 0; }

    // Debug: get frustum for visualization
    const Frustum& getFrustum() const { return m_frustum; }

    // Query buildings within a radius (for player collision)
    void queryRadius(const glm::vec3& center, float radius,
                     std::function<void(const BuildingGenerator::BuildingData&)> callback) const {
        m_octree.queryRadius(center, radius, callback);
    }

    // Raycast for camera collision detection
    // Returns true if ray hits a building, sets hitDist to distance along ray
    // origin: ray start point (look-at position)
    // direction: normalized ray direction (toward camera)
    // maxDist: maximum distance to check (desired camera distance)
    bool raycast(const glm::vec3& origin, const glm::vec3& direction,
                 float maxDist, float& hitDist) const {
        return m_octree.raycast(origin, direction, maxDist, hitDist);
    }

    // Raycast with additional AABB check (e.g., for FING building)
    bool raycastWithExtra(const glm::vec3& origin, const glm::vec3& direction,
                          float maxDist, const AABB* extraAABB, float& hitDist) const {
        float octreeHit = maxDist;
        bool hitOctree = m_octree.raycast(origin, direction, maxDist, octreeHit);

        float extraHit = maxDist;
        bool hitExtra = false;
        if (extraAABB) {
            glm::vec3 dirInv(
                direction.x != 0.0f ? 1.0f / direction.x : 1e30f,
                direction.y != 0.0f ? 1.0f / direction.y : 1e30f,
                direction.z != 0.0f ? 1.0f / direction.z : 1e30f
            );
            hitExtra = extraAABB->raycast(origin, dirInv, maxDist, extraHit);
        }

        if (hitOctree && hitExtra) {
            hitDist = glm::min(octreeHit, extraHit);
            return true;
        } else if (hitOctree) {
            hitDist = octreeHit;
            return true;
        } else if (hitExtra) {
            hitDist = extraHit;
            return true;
        }
        return false;
    }

private:
    const std::vector<BuildingGenerator::BuildingData>* m_buildings = nullptr;
    Octree<BuildingGenerator::BuildingData> m_octree;
    Frustum m_frustum;
    InstancedRenderer m_instancedRenderer;        // For camera view pass
    InstancedRenderer m_shadowInstancedRenderer;  // For shadow pass
    size_t m_visibleCount = 0;
    size_t m_shadowVisibleCount = 0;
};
