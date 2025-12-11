#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

struct AxisRenderer
{
    GLuint vao = 0;
    GLuint vbo = 0;

    void init()
    {
        float vertices[] = {
            // Position          // Color
            // X axis (red)
            0.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
            5.0f, 0.0f, 0.0f,    1.0f, 0.0f, 0.0f,
            // Y axis (green)
            0.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
            0.0f, 5.0f, 0.0f,    0.0f, 1.0f, 0.0f,
            // Z axis (blue)
            0.0f, 0.0f, 0.0f,    0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 5.0f,    0.0f, 0.0f, 1.0f,
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    void draw()
    {
        glBindVertexArray(vao);
        glLineWidth(2.0f);
        glDrawArrays(GL_LINES, 0, 6);
        glBindVertexArray(0);
    }

    void cleanup()
    {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
    }
};

// Wireframe box renderer for debug visualization
struct WireframeBoxRenderer
{
    GLuint vao = 0;
    GLuint vbo = 0;

    void init()
    {
        // Unit box from (0,0,0) to (1,1,1) - will be transformed via model matrix
        // 12 edges = 24 vertices (2 per line)
        float vertices[] = {
            // Bottom face edges
            0.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,   1.0f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f,
            0.0f, 0.0f, 1.0f,   0.0f, 0.0f, 0.0f,
            // Top face edges
            0.0f, 1.0f, 0.0f,   1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,   1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,   0.0f, 1.0f, 1.0f,
            0.0f, 1.0f, 1.0f,   0.0f, 1.0f, 0.0f,
            // Vertical edges
            0.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f,   1.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 1.0f,   1.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 1.0f,   0.0f, 1.0f, 1.0f,
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    // Draw with solid_overlay shader (needs uColor uniform set)
    void draw()
    {
        glBindVertexArray(vao);
        glLineWidth(3.0f);
        glDrawArrays(GL_LINES, 0, 24);
        glBindVertexArray(0);
    }

    void cleanup()
    {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
    }
};
