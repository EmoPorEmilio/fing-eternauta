#pragma once
#include "../Registry.h"
#include "../components/DynamicTerrain.h"
#include "../components/Mesh.h"
#include "../../Shader.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <algorithm>
#include <vector>
#include <utility>

class TerrainSystem {
public:
    void init() {
        m_shader.loadFromFiles("shaders/terrain.vert", "shaders/terrain.frag");
        loadTexture("assets/textures/snow.jpg");
    }

    // Find vertices belonging to left/right foot and cache them
    void findFootVertices(const MeshGroup& meshGroup, const Skeleton& skeleton) {
        m_leftFootVertices.clear();
        m_rightFootVertices.clear();

        // Find ALL foot-related joint indices (foot, toe, etc.)
        std::vector<int> leftFootJoints, rightFootJoints;
        std::cout << "=== Joint names ===" << std::endl;
        for (size_t i = 0; i < skeleton.jointNames.size(); ++i) {
            const std::string& name = skeleton.jointNames[i];
            std::cout << "  [" << i << "] " << name << std::endl;

            // Check for left foot/toe bones (case insensitive matching)
            if (name.find("Left") != std::string::npos || name.find("left") != std::string::npos ||
                name.find("_l") != std::string::npos || name.find(".L") != std::string::npos) {
                if (name.find("Foot") != std::string::npos || name.find("foot") != std::string::npos ||
                    name.find("Toe") != std::string::npos || name.find("toe") != std::string::npos) {
                    leftFootJoints.push_back(static_cast<int>(i));
                }
            }
            // Check for right foot/toe bones
            if (name.find("Right") != std::string::npos || name.find("right") != std::string::npos ||
                name.find("_r") != std::string::npos || name.find(".R") != std::string::npos) {
                if (name.find("Foot") != std::string::npos || name.find("foot") != std::string::npos ||
                    name.find("Toe") != std::string::npos || name.find("toe") != std::string::npos) {
                    rightFootJoints.push_back(static_cast<int>(i));
                }
            }
        }
        std::cout << "Left foot joints: " << leftFootJoints.size() << ", Right: " << rightFootJoints.size() << std::endl;

        // Find the lowest vertices that are influenced by foot bones
        for (const auto& mesh : meshGroup.meshes) {
            for (const auto& v : mesh.skinnedVertices) {
                // Only consider vertices with low Y (near feet in bind pose)
                if (v.position.y > 0.2f) continue;

                // Check if this vertex is influenced by any foot bone
                bool isLeftFoot = false, isRightFoot = false;
                for (int j = 0; j < 4; ++j) {
                    int boneIdx = v.jointIndices[j];
                    float weight = v.weights[j];
                    if (weight < 0.05f) continue;

                    for (int lj : leftFootJoints) {
                        if (boneIdx == lj) isLeftFoot = true;
                    }
                    for (int rj : rightFootJoints) {
                        if (boneIdx == rj) isRightFoot = true;
                    }
                }

                if (isLeftFoot) m_leftFootVertices.push_back(v);
                if (isRightFoot) m_rightFootVertices.push_back(v);
            }
        }

        std::cout << "Found foot vertices: L=" << m_leftFootVertices.size()
                  << ", R=" << m_rightFootVertices.size() << std::endl;
        m_footJointsInitialized = true;
    }

    // Skin a vertex EXACTLY like the GPU shader does
    glm::vec3 skinVertex(const SkinnedVertex& v, const Skeleton& skeleton) {
        // This matches skinned.vert exactly:
        // skinMatrix = w.x * bones[j.x] + w.y * bones[j.y] + w.z * bones[j.z] + w.w * bones[j.w]
        // skinnedPos = skinMatrix * vec4(aPos, 1.0)
        glm::mat4 skinMatrix =
            v.weights.x * skeleton.boneMatrices[v.jointIndices.x] +
            v.weights.y * skeleton.boneMatrices[v.jointIndices.y] +
            v.weights.z * skeleton.boneMatrices[v.jointIndices.z] +
            v.weights.w * skeleton.boneMatrices[v.jointIndices.w];

        glm::vec4 skinnedPos = skinMatrix * glm::vec4(v.position, 1.0f);
        return glm::vec3(skinnedPos);
    }

    // Get foot world position by skinning actual foot vertices
    glm::vec3 getFootWorldPos(const std::vector<SkinnedVertex>& footVerts,
                               const Skeleton& skeleton, const Transform& entityTransform) {
        if (footVerts.empty()) return entityTransform.position;

        // Skin all foot vertices and find centroid with lowest Y
        glm::vec3 centroid(0.0f);
        float lowestY = 999.0f;

        for (const auto& v : footVerts) {
            glm::vec3 skinnedPos = skinVertex(v, skeleton);
            centroid += skinnedPos;
            if (skinnedPos.y < lowestY) lowestY = skinnedPos.y;
        }
        centroid /= static_cast<float>(footVerts.size());

        // Use centroid XZ but lowest Y
        glm::vec3 modelPos(centroid.x, lowestY, centroid.z);

        // Transform to world space: worldPos = uModel * skinnedPos (same as shader)
        glm::vec4 worldPos = entityTransform.matrix() * glm::vec4(modelPos, 1.0f);
        return glm::vec3(worldPos);
    }

    void loadTexture(const char* path) {
        int width, height, channels;
        // Don't flip - most textures don't need it
        unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
        if (data) {
            glGenTextures(1, &m_texture);
            glBindTexture(GL_TEXTURE_2D, m_texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(data);
            std::cout << "Loaded terrain texture: " << path << " (" << width << "x" << height << ")" << std::endl;
        } else {
            std::cerr << "Failed to load terrain texture: " << path << std::endl;
        }
    }

    void update(DynamicTerrain& terrain, Registry& registry, float dt) {
        if (!terrain.initialized) {
            terrain.init();
        }

        // Get follow target position
        if (terrain.followTarget != NULL_ENTITY) {
            auto* targetTransform = registry.getTransform(terrain.followTarget);
            auto* skeleton = registry.getSkeleton(terrain.followTarget);
            auto* meshGroup = registry.getMeshGroup(terrain.followTarget);

            if (targetTransform) {
                glm::vec3 newCenter = targetTransform->position;
                newCenter.y = 0.0f;  // Keep terrain at ground level

                // Check if player moved enough to trigger rebuild
                float dx = newCenter.x - m_currentCenter.x;
                float dz = newCenter.z - m_currentCenter.z;
                float distMoved = dx * dx + dz * dz;

                // Rebuild when player moves more than half a cell
                if (distMoved > (terrain.cellSize * terrain.cellSize * 0.25f)) {
                    m_currentCenter = newCenter;
                    terrain.needsRebuild = true;
                }

                // Find foot vertices once (at startup)
                if (skeleton && meshGroup && !m_footJointsInitialized) {
                    findFootVertices(*meshGroup, *skeleton);
                }

                // Get foot world positions by skinning actual vertices (same as GPU)
                if (skeleton && m_footJointsInitialized) {
                    glm::vec3 leftFootPos = getFootWorldPos(m_leftFootVertices, *skeleton, *targetTransform);
                    glm::vec3 rightFootPos = getFootWorldPos(m_rightFootVertices, *skeleton, *targetTransform);

                    // Store for debug rendering
                    m_debugLeftFoot = leftFootPos;
                    m_debugRightFoot = rightFootPos;

                    // Much bigger footprints for visibility
                    const float deformRadius = 0.5f;      // Large visible footprint
                    const float deformDepth = 0.15f;      // Deeper imprint
                    const float groundThreshold = 0.30f;  // Foot is "on ground" when Y < this (model's feet are around 0.2-0.3 at lowest)

                    // Simple approach: deform continuously while foot is near ground
                    // This ensures every step creates a hole
                    if (leftFootPos.y < groundThreshold) {
                        terrain.deformAt(leftFootPos, deformRadius, deformDepth);
                    }
                    if (rightFootPos.y < groundThreshold) {
                        terrain.deformAt(rightFootPos, deformRadius, deformDepth);
                    }

                    // Print debug info once per second
                    static float debugTimer = 0.0f;
                    debugTimer += dt;
                    if (debugTimer > 1.0f) {
                        debugTimer = 0.0f;
                        std::cout << "Foot pos - L: (" << leftFootPos.x << ", " << leftFootPos.y << ", " << leftFootPos.z << ")"
                                  << " R: (" << rightFootPos.x << ", " << rightFootPos.y << ", " << rightFootPos.z << ")"
                                  << " | Char: (" << targetTransform->position.x << ", " << targetTransform->position.z << ")"
                                  << std::endl;
                    }
                }

                m_lastPlayerPos = targetTransform->position;
            }
        }

        // Rebuild mesh if needed
        if (terrain.needsRebuild) {
            terrain.rebuildMesh(m_currentCenter);
        }
    }

    void render(DynamicTerrain& terrain, Registry& registry, float aspectRatio) {
        if (!terrain.initialized || terrain.vao == 0) return;

        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return;

        auto* cam = registry.getCamera(camEntity);
        auto* camTransform = registry.getTransform(camEntity);
        if (!cam || !camTransform) return;

        // Compute view matrix
        glm::mat4 view;
        auto* followTarget = registry.getFollowTarget(camEntity);
        if (followTarget && followTarget->target != NULL_ENTITY) {
            auto* targetTransform = registry.getTransform(followTarget->target);
            auto* facing = registry.getFacingDirection(followTarget->target);
            if (targetTransform && facing) {
                float yawRad = glm::radians(facing->yaw);
                glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
                glm::vec3 lookAtPos = targetTransform->position + forward * followTarget->lookAhead;
                lookAtPos.y += 1.0f;
                view = glm::lookAt(camTransform->position, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
            } else {
                view = glm::lookAt(camTransform->position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }
        } else {
            view = glm::lookAt(camTransform->position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        renderInternal(terrain, cam, camTransform, view, aspectRatio);
    }

    void renderWithView(DynamicTerrain& terrain, Registry& registry, float aspectRatio, const glm::mat4& view) {
        if (!terrain.initialized || terrain.vao == 0) return;

        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return;

        auto* cam = registry.getCamera(camEntity);
        auto* camTransform = registry.getTransform(camEntity);
        if (!cam || !camTransform) return;

        renderInternal(terrain, cam, camTransform, view, aspectRatio);
    }

    // Render ground plane only where terrain hasn't been drawn (stencil = 0)
    void renderGroundWithStencil(GLuint groundVAO, GLuint groundTexture, Shader& groundShader,
                                  Registry& registry, float aspectRatio, const glm::mat4& view) {
        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return;

        auto* cam = registry.getCamera(camEntity);
        auto* camTransform = registry.getTransform(camEntity);
        if (!cam || !camTransform) return;

        glm::mat4 projection = cam->projectionMatrix(aspectRatio);
        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

        // Only render where stencil is 0 (outside terrain square)
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, 0, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        glStencilMask(0x00);

        groundShader.use();
        groundShader.setMat4("uView", view);
        groundShader.setMat4("uProjection", projection);
        groundShader.setMat4("uModel", model);
        groundShader.setVec3("uLightDir", lightDir);
        groundShader.setVec3("uViewPos", camTransform->position);
        groundShader.setInt("uHasTexture", 1);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundTexture);
        groundShader.setInt("uTexture", 0);

        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);

        glDisable(GL_STENCIL_TEST);
        glStencilMask(0xFF);  // Re-enable stencil writing so glClear can clear it next frame
    }

private:
    void renderInternal(DynamicTerrain& terrain, CameraComponent* cam, Transform* camTransform,
                        const glm::mat4& view, float aspectRatio) {
        glm::mat4 projection = cam->projectionMatrix(aspectRatio);
        glm::mat4 model = glm::mat4(1.0f);  // Identity - terrain positions are already world space
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

        m_shader.use();
        m_shader.setMat4("uView", view);
        m_shader.setMat4("uProjection", projection);
        m_shader.setMat4("uModel", model);
        m_shader.setVec3("uLightDir", lightDir);
        m_shader.setVec3("uViewPos", camTransform->position);
        m_shader.setFloat("uTexScale", 0.5f);  // Texture tiles every 2 units

        // Bind snow texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        m_shader.setInt("uTexture", 0);

        // Enable stencil writing - mark terrain area with 1
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        glStencilMask(0xFF);

        // Disable back-face culling so deformed triangles still render
        glDisable(GL_CULL_FACE);

        glBindVertexArray(terrain.vao);
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(terrain.indices.size()), GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        // Re-enable back-face culling
        glEnable(GL_CULL_FACE);

        // Disable stencil writing
        glStencilMask(0x00);
        glDisable(GL_STENCIL_TEST);
    }

    Shader m_shader;
    GLuint m_texture = 0;
    glm::vec3 m_currentCenter = glm::vec3(0.0f);
    glm::vec3 m_lastPlayerPos = glm::vec3(0.0f);

    // Vertex-based foot tracking (uses exact same skinning as GPU)
    std::vector<SkinnedVertex> m_leftFootVertices;
    std::vector<SkinnedVertex> m_rightFootVertices;
    bool m_footJointsInitialized = false;

    // Debug foot positions
    glm::vec3 m_debugLeftFoot = glm::vec3(0.0f);
    glm::vec3 m_debugRightFoot = glm::vec3(0.0f);
    GLuint m_debugVAO = 0;
    GLuint m_debugVBO = 0;
    Shader m_debugShader;
    bool m_debugInitialized = false;

public:
    // Call this after render to draw debug markers at foot positions
    void renderDebugMarkers(Registry& registry, float aspectRatio, const glm::mat4& view) {
        if (!m_debugInitialized) {
            // Create simple point shader
            const char* vertSrc = R"(
                #version 450 core
                layout (location = 0) in vec3 aPos;
                uniform mat4 uMVP;
                void main() {
                    gl_Position = uMVP * vec4(aPos, 1.0);
                    gl_PointSize = 20.0;
                }
            )";
            const char* fragSrc = R"(
                #version 450 core
                uniform vec3 uColor;
                out vec4 FragColor;
                void main() {
                    FragColor = vec4(uColor, 1.0);
                }
            )";
            m_debugShader.loadFromSource(vertSrc, fragSrc);

            glGenVertexArrays(1, &m_debugVAO);
            glGenBuffers(1, &m_debugVBO);
            glBindVertexArray(m_debugVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
            glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glBindVertexArray(0);

            m_debugInitialized = true;
        }

        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return;
        auto* cam = registry.getCamera(camEntity);
        if (!cam) return;

        glm::mat4 projection = cam->projectionMatrix(aspectRatio);
        glm::mat4 mvp = projection * view;

        // Update vertex buffer with both foot positions (project to ground Y=0.01)
        float verts[6] = {
            m_debugLeftFoot.x, 0.05f, m_debugLeftFoot.z,
            m_debugRightFoot.x, 0.05f, m_debugRightFoot.z
        };
        glBindBuffer(GL_ARRAY_BUFFER, m_debugVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glDisable(GL_DEPTH_TEST);  // Always visible

        m_debugShader.use();
        m_debugShader.setMat4("uMVP", mvp);

        glBindVertexArray(m_debugVAO);

        // Draw left foot marker (green)
        m_debugShader.setVec3("uColor", glm::vec3(0.0f, 1.0f, 0.0f));
        glDrawArrays(GL_POINTS, 0, 1);

        // Draw right foot marker (blue)
        m_debugShader.setVec3("uColor", glm::vec3(0.0f, 0.0f, 1.0f));
        glDrawArrays(GL_POINTS, 1, 1);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
};
