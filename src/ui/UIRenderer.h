#pragma once
#include "../Shader.h"
#include "TextCache.h"
#include "../ecs/components/UIText.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class UIRenderer {
public:
    UIRenderer() = default;

    ~UIRenderer() {
        cleanup();
    }

    bool init() {
        if (!m_shader.loadFromFiles("shaders/ui.vert", "shaders/ui.frag")) {
            return false;
        }

        // Create quad VAO for text rendering
        // Vertices: position (2) + texcoord (2)
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

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glGenBuffers(1, &m_ebo);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // TexCoord attribute
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);

        return true;
    }

    void cleanup() {
        if (m_vao) {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }
        if (m_vbo) {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }
        if (m_ebo) {
            glDeleteBuffers(1, &m_ebo);
            m_ebo = 0;
        }
    }

    void beginFrame(int screenWidth, int screenHeight) {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;

        // Orthographic projection: (0,0) bottom-left, (width, height) top-right
        m_projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    }

    void renderText(const TextTexture& texture, const UIText& uiText) {
        if (!texture.isValid() || !uiText.visible) return;

        // Calculate position based on anchor and alignment
        glm::vec2 anchorPos = anchorToNormalized(uiText.anchor);
        glm::vec2 screenAnchor = glm::vec2(
            anchorPos.x * m_screenWidth,
            anchorPos.y * m_screenHeight
        );

        // Apply offset
        glm::vec2 pos = screenAnchor + uiText.offset;

        // Adjust for horizontal alignment
        switch (uiText.horizontalAlign) {
            case HorizontalAlign::Left:
                // Position is left edge
                break;
            case HorizontalAlign::Center:
                pos.x -= texture.width * 0.5f;
                break;
            case HorizontalAlign::Right:
                pos.x -= texture.width;
                break;
        }

        // Render
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        m_shader.use();
        m_shader.setMat4("uProjection", m_projection);
        m_shader.setVec2("uPosition", pos);
        m_shader.setVec2("uSize", glm::vec2(texture.width, texture.height));
        m_shader.setVec4("uColor", uiText.color / 255.0f);  // Normalize to 0-1
        m_shader.setInt("uTexture", 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture.textureId);

        glBindVertexArray(m_vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }

private:
    Shader m_shader;
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    glm::mat4 m_projection{1.0f};
    int m_screenWidth = 0;
    int m_screenHeight = 0;
};
