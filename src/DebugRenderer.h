#pragma once

#include <glad/glad.h>

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
