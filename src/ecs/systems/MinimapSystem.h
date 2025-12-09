#pragma once
#include "../../Shader.h"
#include "../../ui/FontManager.h"
#include "../../ui/TextCache.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <vector>

class MinimapSystem {
public:
    void init() {
        m_shader.loadFromFiles("shaders/minimap.vert", "shaders/minimap.frag");
        m_markerShader.loadFromFiles("shaders/minimap_marker.vert", "shaders/minimap_marker.frag");
        m_textShader.loadFromFiles("shaders/ui.vert", "shaders/ui.frag");
        m_rectShader.loadFromFiles("shaders/minimap_rect.vert", "shaders/minimap_rect.frag");
        setupQuad();
        setupTextQuad();
        setupRectQuad();
    }

    void render(int screenWidth, int screenHeight, float playerYaw, FontManager& fontManager, TextCache& textCache,
                const glm::vec3& playerPos = glm::vec3(0.0f), const std::vector<glm::vec3>& markerPositions = {},
                const std::vector<std::pair<glm::vec2, glm::vec2>>& buildingFootprints = {}) {
        glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);

        float radius = 80.0f;
        float padding = 20.0f;
        glm::vec2 center(screenWidth - radius - padding, screenHeight - radius - padding);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        // Draw background circle
        m_shader.use();
        m_shader.setMat4("uProjection", projection);
        m_shader.setVec2("uCenter", center);
        m_shader.setFloat("uRadius", radius);
        m_shader.setVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.4f));

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        // Draw building rectangles (before player dot so they appear behind)
        renderBuildingFootprints(projection, center, radius, playerPos, playerYaw, buildingFootprints);

        // Draw entity markers (X markers)
        renderEntityMarkers(projection, center, radius, playerPos, playerYaw, markerPositions, fontManager, textCache);

        // Draw player dot in center (white)
        float dotRadius = 6.0f;
        m_markerShader.use();
        m_markerShader.setMat4("uProjection", projection);
        m_markerShader.setVec2("uCenter", center);
        m_markerShader.setFloat("uRadius", dotRadius);
        m_markerShader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // Draw cardinal points (N, E, S, W) as text rotating based on player yaw
        renderCardinalPoints(projection, center, radius, playerYaw, fontManager, textCache);

        glEnable(GL_DEPTH_TEST);
    }

    // Overload for backward compatibility (GodMode uses this)
    void render(int screenWidth, int screenHeight) {
        // Simplified render without cardinal text - just draw circle and player dot
        glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);

        float radius = 80.0f;
        float padding = 20.0f;
        glm::vec2 center(screenWidth - radius - padding, screenHeight - radius - padding);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        m_shader.use();
        m_shader.setMat4("uProjection", projection);
        m_shader.setVec2("uCenter", center);
        m_shader.setFloat("uRadius", radius);
        m_shader.setVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.4f));

        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        float dotRadius = 6.0f;
        m_markerShader.use();
        m_markerShader.setMat4("uProjection", projection);
        m_markerShader.setVec2("uCenter", center);
        m_markerShader.setFloat("uRadius", dotRadius);
        m_markerShader.setVec4("uColor", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

private:
    Shader m_shader;
    Shader m_markerShader;
    Shader m_textShader;
    Shader m_rectShader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_textVao = 0;
    GLuint m_textVbo = 0;
    GLuint m_textEbo = 0;
    GLuint m_rectVao = 0;
    GLuint m_rectVbo = 0;
    GLuint m_rectEbo = 0;

    void setupQuad() {
        float vertices[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        glBindVertexArray(0);
    }

    void setupTextQuad() {
        float vertices[] = {
            // pos      // tex
            0.0f, 0.0f, 0.0f, 1.0f,  // bottom-left
            1.0f, 0.0f, 1.0f, 1.0f,  // bottom-right
            1.0f, 1.0f, 1.0f, 0.0f,  // top-right
            0.0f, 1.0f, 0.0f, 0.0f   // top-left
        };

        unsigned int indices[] = {
            0, 1, 2,
            0, 2, 3
        };

        glGenVertexArrays(1, &m_textVao);
        glGenBuffers(1, &m_textVbo);
        glGenBuffers(1, &m_textEbo);

        glBindVertexArray(m_textVao);

        glBindBuffer(GL_ARRAY_BUFFER, m_textVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_textEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void setupRectQuad() {
        // Unit quad from (0,0) to (1,1) - will be scaled/transformed per rectangle
        float vertices[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };

        unsigned int indices[] = {
            0, 1, 2,
            0, 2, 3
        };

        glGenVertexArrays(1, &m_rectVao);
        glGenBuffers(1, &m_rectVbo);
        glGenBuffers(1, &m_rectEbo);

        glBindVertexArray(m_rectVao);

        glBindBuffer(GL_ARRAY_BUFFER, m_rectVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rectEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // Minimap world range - how many world units the minimap covers
    static constexpr float MINIMAP_WORLD_RADIUS = 150.0f;  // Configurable: minimap shows 150 world units in each direction

    void renderBuildingFootprints(const glm::mat4& projection, const glm::vec2& center, float radius,
                                   const glm::vec3& playerPos, float playerYaw,
                                   const std::vector<std::pair<glm::vec2, glm::vec2>>& footprints) {
        if (footprints.empty()) return;

        float mapScale = radius / MINIMAP_WORLD_RADIUS;  // Scale world units to minimap pixels
        float rotationRad = glm::radians(-playerYaw);
        float cosR = std::cos(rotationRad);
        float sinR = std::sin(rotationRad);

        m_rectShader.use();
        m_rectShader.setMat4("uProjection", projection);
        m_rectShader.setVec2("uMinimapCenter", center);
        m_rectShader.setFloat("uMinimapRadius", radius);
        m_rectShader.setVec4("uColor", glm::vec4(0.3f, 0.3f, 0.3f, 0.9f));  // Dark gray buildings

        glBindVertexArray(m_rectVao);

        for (const auto& [bldgCenter, halfExtents] : footprints) {
            // Calculate relative position from player
            float relX = bldgCenter.x - playerPos.x;
            float relZ = bldgCenter.y - playerPos.z;

            // Skip buildings outside minimap world range (optimization for large grids)
            float distSq = relX * relX + relZ * relZ;
            if (distSq > MINIMAP_WORLD_RADIUS * MINIMAP_WORLD_RADIUS * 1.5f) continue;  // 1.5x margin for buildings at edges

            // Rotate to align with player view direction
            float rotatedCenterX = relX * cosR + relZ * sinR;
            float rotatedCenterZ = -relX * sinR + relZ * cosR;

            // Scale to minimap coordinates
            float mapCenterX = center.x + rotatedCenterX * mapScale;
            float mapCenterY = center.y - rotatedCenterZ * mapScale;

            // Rotated half extents (width and depth swap based on rotation)
            float scaledHalfW = halfExtents.x * mapScale;
            float scaledHalfD = halfExtents.y * mapScale;

            // Pass rect position, size and rotation angle to shader
            m_rectShader.setVec2("uRectCenter", glm::vec2(mapCenterX, mapCenterY));
            m_rectShader.setVec2("uRectHalfSize", glm::vec2(scaledHalfW, scaledHalfD));
            m_rectShader.setFloat("uRotation", rotationRad);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
    }

    void renderCardinalPoints(const glm::mat4& projection, const glm::vec2& center, float radius, float playerYaw,
                              FontManager& fontManager, TextCache& textCache) {
        float cardinalRadius = radius * 0.80f;
        float rotationRad = glm::radians(-playerYaw);

        Font* font = fontManager.getFont("oxanium_small", 17);
        if (!font) return;

        struct CardinalPoint {
            const char* letter;
            float baseAngle;
            TextStyle style;
        };

        // N=0 (top), E=90 (right), S=180 (bottom), W=270 (left)
        CardinalPoint cardinals[] = {
            {"N", 0.0f,   {255, 100, 100, 255}},  // North - reddish
            {"E", 90.0f,  {255, 255, 255, 200}},  // East - white
            {"S", 180.0f, {255, 255, 255, 200}},  // South - white
            {"W", 270.0f, {255, 255, 255, 200}}   // West - white
        };

        m_textShader.use();
        m_textShader.setMat4("uProjection", projection);
        m_textShader.setInt("uTexture", 0);

        for (const auto& cardinal : cardinals) {
            float angleRad = glm::radians(cardinal.baseAngle) + rotationRad;

            float x = center.x + std::sin(angleRad) * cardinalRadius;
            float y = center.y + std::cos(angleRad) * cardinalRadius;

            TextTexture texture = textCache.render(*font, cardinal.letter, cardinal.style);
            if (!texture.isValid()) continue;

            // Center the text at position
            glm::vec2 pos(x - texture.width * 0.5f, y - texture.height * 0.5f);

            m_textShader.setVec2("uPosition", pos);
            m_textShader.setVec2("uSize", glm::vec2(texture.width, texture.height));
            m_textShader.setVec4("uColor", glm::vec4(cardinal.style.r, cardinal.style.g, cardinal.style.b, cardinal.style.a) / 255.0f);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture.textureId);

            glBindVertexArray(m_textVao);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
    }

    void renderEntityMarkers(const glm::mat4& projection, const glm::vec2& center, float radius,
                             const glm::vec3& playerPos, float playerYaw, const std::vector<glm::vec3>& markerPositions,
                             FontManager& fontManager, TextCache& textCache) {
        if (markerPositions.empty()) return;

        float mapScale = 2.0f;  // 1 world unit = 2 pixels on minimap (adjustable)
        float rotationRad = glm::radians(-playerYaw);

        Font* font = fontManager.getFont("oxanium_bold", 20);
        if (!font) {
            font = fontManager.getFont("oxanium_small", 17);
            if (!font) return;
        }

        TextStyle markerStyle = {100, 255, 100, 255};  // Green color
        TextTexture texture = textCache.render(*font, "X", markerStyle);
        if (!texture.isValid()) return;

        m_textShader.use();
        m_textShader.setMat4("uProjection", projection);
        m_textShader.setInt("uTexture", 0);
        m_textShader.setVec2("uSize", glm::vec2(texture.width, texture.height));
        m_textShader.setVec4("uColor", glm::vec4(markerStyle.r, markerStyle.g, markerStyle.b, markerStyle.a) / 255.0f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.textureId);
        glBindVertexArray(m_textVao);

        for (const auto& worldPos : markerPositions) {
            // Calculate relative position from player (XZ plane)
            float relX = worldPos.x - playerPos.x;
            float relZ = worldPos.z - playerPos.z;

            // Rotate to align with player view direction
            float rotatedX =  relX * std::cos(rotationRad) + relZ * std::sin(rotationRad);
            float rotatedZ = -relX * std::sin(rotationRad) + relZ * std::cos(rotationRad);

            // Scale to minimap coordinates
            float mapX = rotatedX * mapScale;
            float mapY = -rotatedZ * mapScale;

            // Calculate distance from center
            float dist = std::sqrt(mapX * mapX + mapY * mapY);

            // Clamp to circle boundary (with margin for text size)
            float maxDist = radius - texture.width * 0.5f - 2.0f;
            if (dist > maxDist) {
                float scale = maxDist / dist;
                mapX *= scale;
                mapY *= scale;
            }

            // Convert to screen coordinates (centered on the X)
            glm::vec2 pos(center.x + mapX - texture.width * 0.5f, center.y + mapY - texture.height * 0.5f);

            m_textShader.setVec2("uPosition", pos);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
    }
};
