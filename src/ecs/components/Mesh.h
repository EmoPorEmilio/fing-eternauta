#pragma once
#include <glad/glad.h>
#include <vector>

struct Mesh {
    GLuint vao = 0;
    GLsizei indexCount = 0;
    GLenum indexType = GL_UNSIGNED_SHORT;
    bool hasSkinning = false;
    GLuint texture = 0;
};

struct MeshGroup {
    std::vector<Mesh> meshes;
};
