#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <map>

// =============================================================================
// Model - Loads and renders a GLB/GLTF model with textures and skeletal animation
// =============================================================================

// Maximum bones per vertex (GLTF typically uses 4)
constexpr int MAX_BONE_INFLUENCE = 4;
constexpr int MAX_BONES = 128;

struct Mesh
{
    GLuint vao = 0;
    GLuint vboPositions = 0;
    GLuint vboNormals = 0;
    GLuint vboTexCoords = 0;
    GLuint vboJoints = 0;      // Joint indices (4 per vertex)
    GLuint vboWeights = 0;     // Joint weights (4 per vertex)
    GLuint ebo = 0;
    GLsizei indexCount = 0;
    GLenum indexType = GL_UNSIGNED_SHORT;
    int textureIndex = -1;
    bool hasSkinning = false;
};

struct Joint
{
    std::string name;
    int parentIndex = -1;
    glm::mat4 inverseBindMatrix = glm::mat4(1.0f);
    glm::mat4 localTransform = glm::mat4(1.0f);
};

struct AnimationChannel
{
    int jointIndex = -1;
    std::vector<float> translationTimes;
    std::vector<glm::vec3> translations;
    std::vector<float> rotationTimes;
    std::vector<glm::quat> rotations;
    std::vector<float> scaleTimes;
    std::vector<glm::vec3> scales;
};

struct Animation
{
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationChannel> channels;
};

class Model
{
public:
    Model() = default;
    ~Model();

    // Non-copyable
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Movable
    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    // Load from GLB file
    bool loadFromFile(const std::string& path);

    // Update animation (call each frame)
    void updateAnimation(float deltaTime);

    // Set current animation by index
    void setAnimation(int index);

    // Render all meshes
    void draw() const;

    // Bind texture for a mesh
    void bindTexture(int meshIndex, GLenum textureUnit = GL_TEXTURE0) const;

    // Check if model has textures
    bool hasTextures() const { return !m_textures.empty(); }

    // Check if model has animations
    bool hasAnimations() const { return !m_animations.empty(); }

    // Get bone matrices for shader
    const std::vector<glm::mat4>& getBoneMatrices() const { return m_boneMatrices; }

    size_t getMeshCount() const { return m_meshes.size(); }
    size_t getAnimationCount() const { return m_animations.size(); }

private:
    std::vector<Mesh> m_meshes;
    std::vector<GLuint> m_textures;

    // Skeleton data
    std::vector<Joint> m_joints;
    std::vector<int> m_jointNodeIndices;  // Maps joint index to node index
    std::map<int, int> m_nodeToJoint;     // Maps node index to joint index

    // Animation data
    std::vector<Animation> m_animations;
    int m_currentAnimation = 0;
    float m_animationTime = 0.0f;

    // Final bone matrices (uploaded to shader)
    std::vector<glm::mat4> m_boneMatrices;

    void cleanup();
    void computeBoneMatrices();
    glm::mat4 interpolateTransform(const AnimationChannel& channel, float time);
};
