#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <memory>

// Forward declaration
namespace tinygltf
{
    class TinyGLTF;
    class Model;
    struct Primitive;
    struct Material;
    struct Texture;
    struct Image;
}

struct DrawablePrimitive
{
    std::vector<GLfloat> vertices;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> colors;
    std::vector<GLfloat> uvs;
    std::vector<GLfloat> tangents;            // xyz tangent and w sign per vertex (store as vec4)
    std::vector<unsigned short> jointIndices; // 4 per vertex (indices into skin joints)
    std::vector<GLfloat> jointWeights;        // 4 per vertex
    std::vector<unsigned int> indices;
    int glMode = GL_TRIANGLES;

    // OpenGL objects
    GLuint vao = 0;
    GLuint vbo[7] = {0, 0, 0, 0, 0, 0, 0}; // vertices, normals, colors, uvs, tangents, weights, joints
    GLuint ebo = 0;

    // Textures
    GLuint baseColorTexture = 0;
    GLuint metallicRoughnessTexture = 0;
    GLuint occlusionTexture = 0;
    GLuint normalTexture = 0;

    // Material properties
    glm::vec4 baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float metallicFactor = 0.0f;
    float roughnessFactor = 0.6f;
    float occlusionStrength = 1.0f;
    float normalScale = 1.0f;

    // Flags
    bool hasBaseColorTexture = false;
    bool hasMetallicRoughnessTexture = false;
    bool hasOcclusionTexture = false;
    bool hasNormalTexture = false;
    bool isInitialized = false;

    // Skinning
    int skinIndex = -1;                        // -1 if not skinned; otherwise index into GLTFModel skins
    glm::mat4 nodeTransform = glm::mat4(1.0f); // mesh node transform (applied in shader path)

    // Cleanup
    void cleanup();
};

class GLTFModel
{
public:
    GLTFModel();
    ~GLTFModel();

    // Loading
    bool loadFromFile(const std::string &filepath);
    void cleanup();

    // Rendering
    void render(const glm::mat4 &view, const glm::mat4 &projection,
                const glm::vec3 &cameraPos, const glm::vec3 &lightDir,
                const glm::vec3 &lightColor, GLuint shaderProgram);

    // Animation control (MVP)
    void setAnimationTime(float seconds) { m_animTime = seconds; }
    float getAnimationTime() const { return m_animTime; }
    void advanceAnimation(float deltaTime) { m_animTime += deltaTime; }
    void setAnimationEnabled(bool enabled) { m_animEnabled = enabled; }

    // Transform
    void setTransform(const glm::mat4 &transform) { m_transform = transform; }
    glm::mat4 getTransform() const { return m_transform; }

    // Getters
    bool isLoaded() const { return m_isLoaded; }
    size_t getPrimitiveCount() const { return m_primitives.size(); }
    size_t getVertexCount() const;
    size_t getTriangleCount() const;

    // Bounding box
    glm::vec3 getMinBounds() const { return m_minBounds; }
    glm::vec3 getMaxBounds() const { return m_maxBounds; }
    glm::vec3 getCenter() const;
    float getRadius() const;

private:
    std::vector<DrawablePrimitive> m_primitives;
    glm::mat4 m_transform;
    bool m_isLoaded;

    // Bounding box
    glm::vec3 m_minBounds;
    glm::vec3 m_maxBounds;

    // Helper functions
    bool loadGLB(const std::string &filepath);
    void processNode(const tinygltf::Model &model, int nodeIndex, const glm::mat4 &parentTransform);
    void processMesh(const tinygltf::Model &model, int meshIndex, const glm::mat4 &transform, int skinIndex);
    void processPrimitive(const tinygltf::Model &model, const tinygltf::Primitive &primitive,
                          const glm::mat4 &transform, int materialIndex, int skinIndex);

    // Texture loading
    GLuint loadTexture(const tinygltf::Model &model, int textureIndex, bool srgb);

    // Buffer setup
    void setupPrimitive(DrawablePrimitive &primitive);

    // Bounds calculation
    void calculateBounds();

    // Skinning data per model
    struct SkinData
    {
        std::vector<int> joints;            // node indices
        std::vector<glm::mat4> inverseBind; // one per joint
    };
    std::vector<SkinData> m_skins;
    std::vector<glm::mat4> m_nodeGlobalTransforms; // indexed by node index

    // Node hierarchy for animation
    std::vector<int> m_nodeParents;          // parent index per node (-1 for roots)
    std::vector<std::vector<int>> m_nodeChildren; // children per node
    std::vector<int> m_sceneRoots;           // root nodes of scene 0

    // Base local TRS per node (for animation blending)
    std::vector<glm::vec3> m_nodeBaseT;
    std::vector<glm::quat> m_nodeBaseR;
    std::vector<glm::vec3> m_nodeBaseS;

    // Simple animation structures (first clip only)
    enum AnimPath
    {
        PATH_T,
        PATH_R,
        PATH_S
    };
    struct AnimSampler
    {
        std::vector<float> input;      // keyframe times
        std::vector<glm::vec4> output; // vec3 in xyz for T/S, quat in xyzw for R
        int components = 0;            // 3 for T/S, 4 for R
        std::string interpolation;     // "LINEAR"/"STEP"/"CUBICSPLINE" (use linear/step)
    };
    struct AnimChannel
    {
        int samplerIndex;
        int targetNode;
        AnimPath path;
    };
    struct AnimationClip
    {
        std::vector<AnimSampler> samplers;
        std::vector<AnimChannel> channels;
        float duration = 0.0f;
    };
    std::vector<AnimationClip> m_animations;
    float m_animTime = 0.0f;
    bool m_animEnabled = false;

    void evaluateAnimation(float timeSeconds);
};
