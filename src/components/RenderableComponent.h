#pragma once

#include <glad/glad.h>
#include <cstdint>

namespace ecs {

// Renderable type determines which render path to use
enum class RenderableType : uint8_t {
    INSTANCED_PRISM,  // ObjectManager prisms (GPU instanced)
    GLTF_MODEL,       // PBR models
    PARTICLE,         // Billboard particles (snow)
    FLOOR,            // Floor plane
    CUSTOM            // Custom rendering
};

struct RenderableComponent {
    RenderableType type = RenderableType::CUSTOM;

    // OpenGL handles (optional, depends on type)
    GLuint vao = 0;
    GLuint shaderProgram = 0;

    // Visibility state (updated by CullingSystem)
    bool visible = true;

    // Mesh/geometry identifier (type-specific meaning)
    // For INSTANCED_PRISM: unused (geometry is shared)
    // For GLTF_MODEL: model index in ModelManager
    // For PARTICLE: particle template index
    uint32_t meshId = 0;

    // Layer for render ordering (0 = default, higher = later)
    uint8_t layer = 0;

    // Cast/receive shadows
    bool castShadow = true;
    bool receiveShadow = true;

    RenderableComponent() = default;

    explicit RenderableComponent(RenderableType t)
        : type(t) {}

    RenderableComponent(RenderableType t, GLuint shader)
        : type(t), shaderProgram(shader) {}
};

} // namespace ecs
