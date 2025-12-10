#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// CPU-side skinning data for a single vertex
struct SkinnedVertex {
    glm::vec3 position;      // Bind pose position
    glm::ivec4 jointIndices; // Which bones affect this vertex
    glm::vec4 weights;       // How much each bone affects it
};

struct Mesh {
    GLuint vao = 0;
    GLsizei indexCount = 0;
    GLenum indexType = GL_UNSIGNED_SHORT;
    bool hasSkinning = false;
    GLuint texture = 0;
    GLuint normalMap = 0;  // Normal map texture

    // CPU-side vertex data for skinning calculations
    std::vector<SkinnedVertex> skinnedVertices;
};

struct MeshGroup {
    std::vector<Mesh> meshes;
};
