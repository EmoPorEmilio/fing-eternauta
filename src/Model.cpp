#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include "Model.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <algorithm>

Model::~Model()
{
    cleanup();
}

Model::Model(Model&& other) noexcept
    : m_meshes(std::move(other.m_meshes))
    , m_textures(std::move(other.m_textures))
    , m_joints(std::move(other.m_joints))
    , m_jointNodeIndices(std::move(other.m_jointNodeIndices))
    , m_nodeToJoint(std::move(other.m_nodeToJoint))
    , m_animations(std::move(other.m_animations))
    , m_currentAnimation(other.m_currentAnimation)
    , m_animationTime(other.m_animationTime)
    , m_boneMatrices(std::move(other.m_boneMatrices))
{
}

Model& Model::operator=(Model&& other) noexcept
{
    if (this != &other)
    {
        cleanup();
        m_meshes = std::move(other.m_meshes);
        m_textures = std::move(other.m_textures);
        m_joints = std::move(other.m_joints);
        m_jointNodeIndices = std::move(other.m_jointNodeIndices);
        m_nodeToJoint = std::move(other.m_nodeToJoint);
        m_animations = std::move(other.m_animations);
        m_currentAnimation = other.m_currentAnimation;
        m_animationTime = other.m_animationTime;
        m_boneMatrices = std::move(other.m_boneMatrices);
    }
    return *this;
}

void Model::cleanup()
{
    for (auto& mesh : m_meshes)
    {
        if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
        if (mesh.vboPositions) glDeleteBuffers(1, &mesh.vboPositions);
        if (mesh.vboNormals) glDeleteBuffers(1, &mesh.vboNormals);
        if (mesh.vboTexCoords) glDeleteBuffers(1, &mesh.vboTexCoords);
        if (mesh.vboJoints) glDeleteBuffers(1, &mesh.vboJoints);
        if (mesh.vboWeights) glDeleteBuffers(1, &mesh.vboWeights);
        if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
    }
    m_meshes.clear();

    if (!m_textures.empty())
        glDeleteTextures(static_cast<GLsizei>(m_textures.size()), m_textures.data());
    m_textures.clear();

    m_joints.clear();
    m_animations.clear();
    m_boneMatrices.clear();
}

// Helper to get byte size of a GLTF component type
static size_t getComponentByteSize(int componentType)
{
    switch (componentType)
    {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            return 1;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            return 2;
        case TINYGLTF_COMPONENT_TYPE_INT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            return 4;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            return 8;
        default:
            return 0;
    }
}

// Helper to get number of components for a GLTF type
static int getNumComponents(int type)
{
    switch (type)
    {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2:   return 2;
        case TINYGLTF_TYPE_VEC3:   return 3;
        case TINYGLTF_TYPE_VEC4:   return 4;
        case TINYGLTF_TYPE_MAT2:   return 4;
        case TINYGLTF_TYPE_MAT3:   return 9;
        case TINYGLTF_TYPE_MAT4:   return 16;
        default:                   return 0;
    }
}

// Helper to get accessor data pointer
static const unsigned char* getAccessorData(const tinygltf::Model& model, int accessorIndex, size_t& outCount, size_t& outStride)
{
    const auto& accessor = model.accessors[accessorIndex];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    outCount = accessor.count;
    outStride = bufferView.byteStride;
    if (outStride == 0)
        outStride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

    return &buffer.data[bufferView.byteOffset + accessor.byteOffset];
}

// Helper to get node's local transform matrix
static glm::mat4 getNodeTransform(const tinygltf::Node& node)
{
    if (!node.matrix.empty())
    {
        return glm::make_mat4(node.matrix.data());
    }

    glm::mat4 T = glm::mat4(1.0f);
    glm::mat4 R = glm::mat4(1.0f);
    glm::mat4 S = glm::mat4(1.0f);

    if (!node.translation.empty())
    {
        T = glm::translate(glm::mat4(1.0f), glm::vec3(
            (float)node.translation[0],
            (float)node.translation[1],
            (float)node.translation[2]
        ));
    }

    if (!node.rotation.empty())
    {
        glm::quat q((float)node.rotation[3], (float)node.rotation[0],
                    (float)node.rotation[1], (float)node.rotation[2]);
        R = glm::toMat4(q);
    }

    if (!node.scale.empty())
    {
        S = glm::scale(glm::mat4(1.0f), glm::vec3(
            (float)node.scale[0],
            (float)node.scale[1],
            (float)node.scale[2]
        ));
    }

    return T * R * S;
}

bool Model::loadFromFile(const std::string& path)
{
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool success = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);

    if (!warn.empty())
        std::cerr << "GLTF Warning: " << warn << std::endl;

    if (!err.empty())
        std::cerr << "GLTF Error: " << err << std::endl;

    if (!success)
    {
        std::cerr << "Failed to load GLB: " << path << std::endl;
        return false;
    }

    std::cout << "Loaded GLB: " << path << std::endl;
    std::cout << "  Meshes: " << gltfModel.meshes.size() << std::endl;
    std::cout << "  Textures: " << gltfModel.textures.size() << std::endl;
    std::cout << "  Animations: " << gltfModel.animations.size() << std::endl;
    std::cout << "  Skins: " << gltfModel.skins.size() << std::endl;

    // Load textures
    for (const auto& image : gltfModel.images)
    {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = GL_RGBA;
        if (image.component == 3)
            format = GL_RGB;

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            format,
            image.width,
            image.height,
            0,
            format,
            GL_UNSIGNED_BYTE,
            image.image.data()
        );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        m_textures.push_back(textureID);
    }

    // Load skeleton (skins)
    if (!gltfModel.skins.empty())
    {
        const auto& skin = gltfModel.skins[0];  // Use first skin

        m_joints.resize(skin.joints.size());
        m_jointNodeIndices = skin.joints;

        // Build node to joint mapping
        for (size_t i = 0; i < skin.joints.size(); ++i)
        {
            m_nodeToJoint[skin.joints[i]] = static_cast<int>(i);
        }

        // Load inverse bind matrices
        if (skin.inverseBindMatrices >= 0)
        {
            size_t count, stride;
            const unsigned char* data = getAccessorData(gltfModel, skin.inverseBindMatrices, count, stride);

            for (size_t i = 0; i < count && i < m_joints.size(); ++i)
            {
                const float* matData = reinterpret_cast<const float*>(data + i * stride);
                m_joints[i].inverseBindMatrix = glm::make_mat4(matData);
            }
        }

        // Set joint names and find parent indices
        for (size_t i = 0; i < skin.joints.size(); ++i)
        {
            int nodeIndex = skin.joints[i];
            const auto& node = gltfModel.nodes[nodeIndex];
            m_joints[i].name = node.name;
            m_joints[i].localTransform = getNodeTransform(node);

            // Find parent by checking which node has this as a child
            m_joints[i].parentIndex = -1;
            for (size_t j = 0; j < skin.joints.size(); ++j)
            {
                int parentNodeIndex = skin.joints[j];
                const auto& parentNode = gltfModel.nodes[parentNodeIndex];
                for (int childIdx : parentNode.children)
                {
                    if (childIdx == nodeIndex)
                    {
                        m_joints[i].parentIndex = static_cast<int>(j);
                        break;
                    }
                }
            }
        }

        std::cout << "  Loaded skeleton with " << m_joints.size() << " joints" << std::endl;

        // Initialize bone matrices
        m_boneMatrices.resize(m_joints.size(), glm::mat4(1.0f));
    }

    // Load animations
    for (const auto& gltfAnim : gltfModel.animations)
    {
        Animation anim;
        anim.name = gltfAnim.name;

        for (const auto& channel : gltfAnim.channels)
        {
            int targetNode = channel.target_node;

            // Check if this node is a joint
            auto it = m_nodeToJoint.find(targetNode);
            if (it == m_nodeToJoint.end())
                continue;

            int jointIndex = it->second;

            // Find or create channel for this joint
            AnimationChannel* animChannel = nullptr;
            for (auto& ch : anim.channels)
            {
                if (ch.jointIndex == jointIndex)
                {
                    animChannel = &ch;
                    break;
                }
            }
            if (!animChannel)
            {
                anim.channels.push_back({});
                animChannel = &anim.channels.back();
                animChannel->jointIndex = jointIndex;
            }

            const auto& sampler = gltfAnim.samplers[channel.sampler];

            // Load timestamps
            size_t timeCount, timeStride;
            const unsigned char* timeData = getAccessorData(gltfModel, sampler.input, timeCount, timeStride);
            std::vector<float> times(timeCount);
            for (size_t i = 0; i < timeCount; ++i)
            {
                times[i] = *reinterpret_cast<const float*>(timeData + i * timeStride);
                anim.duration = std::max(anim.duration, times[i]);
            }

            // Load keyframes based on path
            size_t count, stride;
            const unsigned char* data = getAccessorData(gltfModel, sampler.output, count, stride);

            if (channel.target_path == "translation")
            {
                animChannel->translationTimes = times;
                animChannel->translations.resize(count);
                for (size_t i = 0; i < count; ++i)
                {
                    const float* v = reinterpret_cast<const float*>(data + i * stride);
                    animChannel->translations[i] = glm::vec3(v[0], v[1], v[2]);
                }
            }
            else if (channel.target_path == "rotation")
            {
                animChannel->rotationTimes = times;
                animChannel->rotations.resize(count);
                for (size_t i = 0; i < count; ++i)
                {
                    const float* v = reinterpret_cast<const float*>(data + i * stride);
                    animChannel->rotations[i] = glm::quat(v[3], v[0], v[1], v[2]);
                }
            }
            else if (channel.target_path == "scale")
            {
                animChannel->scaleTimes = times;
                animChannel->scales.resize(count);
                for (size_t i = 0; i < count; ++i)
                {
                    const float* v = reinterpret_cast<const float*>(data + i * stride);
                    animChannel->scales[i] = glm::vec3(v[0], v[1], v[2]);
                }
            }
        }

        m_animations.push_back(std::move(anim));
        std::cout << "  Animation '" << m_animations.back().name << "' duration: " << m_animations.back().duration << "s" << std::endl;
    }

    // Process meshes
    for (const auto& gltfMesh : gltfModel.meshes)
    {
        for (const auto& primitive : gltfMesh.primitives)
        {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                std::cerr << "Skipping non-triangle primitive" << std::endl;
                continue;
            }

            Mesh mesh;

            // Create VAO
            glGenVertexArrays(1, &mesh.vao);
            glBindVertexArray(mesh.vao);

            // Positions (required)
            {
                auto it = primitive.attributes.find("POSITION");
                if (it == primitive.attributes.end())
                {
                    std::cerr << "Mesh missing POSITION attribute" << std::endl;
                    glBindVertexArray(0);
                    continue;
                }

                const auto& accessor = gltfModel.accessors[it->second];
                const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                const auto& buffer = gltfModel.buffers[bufferView.buffer];

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t dataSize = bufferView.byteLength;

                int stride = bufferView.byteStride;
                if (stride == 0)
                    stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                glGenBuffers(1, &mesh.vboPositions);
                glBindBuffer(GL_ARRAY_BUFFER, mesh.vboPositions);
                glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);
            }

            // Normals (optional)
            {
                auto it = primitive.attributes.find("NORMAL");
                if (it != primitive.attributes.end())
                {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    size_t dataSize = bufferView.byteLength;

                    int stride = bufferView.byteStride;
                    if (stride == 0)
                        stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                    glGenBuffers(1, &mesh.vboNormals);
                    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboNormals);
                    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                    glEnableVertexAttribArray(1);
                }
            }

            // Texture coordinates (optional)
            {
                auto it = primitive.attributes.find("TEXCOORD_0");
                if (it != primitive.attributes.end())
                {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    size_t dataSize = bufferView.byteLength;

                    int stride = bufferView.byteStride;
                    if (stride == 0)
                        stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                    glGenBuffers(1, &mesh.vboTexCoords);
                    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboTexCoords);
                    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

                    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
                    glEnableVertexAttribArray(2);
                }
            }

            // Joint indices (JOINTS_0)
            {
                auto it = primitive.attributes.find("JOINTS_0");
                if (it != primitive.attributes.end())
                {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    size_t dataSize = accessor.count * 4 * sizeof(float);

                    // Convert to float for shader (joints can be unsigned byte or short)
                    std::vector<float> jointData(accessor.count * 4);
                    int stride = bufferView.byteStride;
                    size_t elemSize = getComponentByteSize(accessor.componentType);
                    if (stride == 0)
                        stride = 4 * elemSize;

                    for (size_t i = 0; i < accessor.count; ++i)
                    {
                        const unsigned char* ptr = dataPtr + i * stride;
                        for (int j = 0; j < 4; ++j)
                        {
                            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                                jointData[i * 4 + j] = static_cast<float>(ptr[j]);
                            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                                jointData[i * 4 + j] = static_cast<float>(reinterpret_cast<const unsigned short*>(ptr)[j]);
                        }
                    }

                    glGenBuffers(1, &mesh.vboJoints);
                    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboJoints);
                    glBufferData(GL_ARRAY_BUFFER, jointData.size() * sizeof(float), jointData.data(), GL_STATIC_DRAW);

                    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
                    glEnableVertexAttribArray(3);

                    mesh.hasSkinning = true;
                }
            }

            // Joint weights (WEIGHTS_0)
            {
                auto it = primitive.attributes.find("WEIGHTS_0");
                if (it != primitive.attributes.end())
                {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    size_t dataSize = bufferView.byteLength;

                    int stride = bufferView.byteStride;
                    if (stride == 0)
                        stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                    glGenBuffers(1, &mesh.vboWeights);
                    glBindBuffer(GL_ARRAY_BUFFER, mesh.vboWeights);
                    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

                    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
                    glEnableVertexAttribArray(4);
                }
            }

            // Indices
            if (primitive.indices >= 0)
            {
                const auto& accessor = gltfModel.accessors[primitive.indices];
                const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                const auto& buffer = gltfModel.buffers[bufferView.buffer];

                mesh.indexCount = static_cast<GLsizei>(accessor.count);

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t elementSize = getComponentByteSize(accessor.componentType);
                size_t dataSize = accessor.count * elementSize;

                glGenBuffers(1, &mesh.ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    mesh.indexType = GL_UNSIGNED_INT;
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    mesh.indexType = GL_UNSIGNED_SHORT;
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    mesh.indexType = GL_UNSIGNED_BYTE;
            }

            // Get texture index from material
            if (primitive.material >= 0)
            {
                const auto& material = gltfModel.materials[primitive.material];
                int texIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                if (texIndex >= 0 && texIndex < static_cast<int>(gltfModel.textures.size()))
                {
                    int imageIndex = gltfModel.textures[texIndex].source;
                    if (imageIndex >= 0 && imageIndex < static_cast<int>(m_textures.size()))
                    {
                        mesh.textureIndex = imageIndex;
                    }
                }
            }

            glBindVertexArray(0);
            m_meshes.push_back(mesh);
        }
    }

    std::cout << "  Total meshes loaded: " << m_meshes.size() << std::endl;

    // Compute initial pose
    if (!m_joints.empty())
        computeBoneMatrices();

    return !m_meshes.empty();
}

void Model::setAnimation(int index)
{
    if (index >= 0 && index < static_cast<int>(m_animations.size()))
    {
        m_currentAnimation = index;
        m_animationTime = 0.0f;
    }
}

// Helper to find keyframe indices and interpolation factor
static bool findKeyframes(const std::vector<float>& times, float t, size_t& i0, size_t& i1, float& factor)
{
    if (times.empty()) return false;
    if (times.size() == 1)
    {
        i0 = i1 = 0;
        factor = 0.0f;
        return true;
    }

    // Before first keyframe
    if (t <= times[0])
    {
        i0 = i1 = 0;
        factor = 0.0f;
        return true;
    }

    for (size_t i = 0; i < times.size() - 1; ++i)
    {
        if (t >= times[i] && t <= times[i + 1])
        {
            i0 = i;
            i1 = i + 1;
            float dt = times[i1] - times[i0];
            factor = (dt > 0.0f) ? (t - times[i0]) / dt : 0.0f;
            return true;
        }
    }

    // Past end, use last keyframe
    i0 = i1 = times.size() - 1;
    factor = 0.0f;
    return true;
}

glm::mat4 Model::interpolateTransform(const AnimationChannel& channel, float time)
{
    glm::vec3 translation(0.0f);
    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale(1.0f);

    // Interpolate translation
    if (!channel.translations.empty() && !channel.translationTimes.empty())
    {
        size_t i0, i1;
        float factor;
        if (findKeyframes(channel.translationTimes, time, i0, i1, factor))
        {
            if (i0 < channel.translations.size() && i1 < channel.translations.size())
            {
                translation = glm::mix(channel.translations[i0], channel.translations[i1], factor);
            }
        }
    }

    // Interpolate rotation (spherical linear interpolation)
    if (!channel.rotations.empty() && !channel.rotationTimes.empty())
    {
        size_t i0, i1;
        float factor;
        if (findKeyframes(channel.rotationTimes, time, i0, i1, factor))
        {
            if (i0 < channel.rotations.size() && i1 < channel.rotations.size())
            {
                rotation = glm::slerp(channel.rotations[i0], channel.rotations[i1], factor);
            }
        }
    }

    // Interpolate scale
    if (!channel.scales.empty() && !channel.scaleTimes.empty())
    {
        size_t i0, i1;
        float factor;
        if (findKeyframes(channel.scaleTimes, time, i0, i1, factor))
        {
            if (i0 < channel.scales.size() && i1 < channel.scales.size())
            {
                scale = glm::mix(channel.scales[i0], channel.scales[i1], factor);
            }
        }
    }

    glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 R = glm::toMat4(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

    return T * R * S;
}

void Model::updateAnimation(float deltaTime)
{
    if (m_animations.empty() || m_joints.empty())
        return;

    const auto& anim = m_animations[m_currentAnimation];

    // Update time (loop animation)
    m_animationTime += deltaTime;
    if (anim.duration > 0.0f)
        m_animationTime = fmod(m_animationTime, anim.duration);

    // Update joint local transforms from animation
    for (const auto& channel : anim.channels)
    {
        if (channel.jointIndex >= 0 && channel.jointIndex < static_cast<int>(m_joints.size()))
        {
            m_joints[channel.jointIndex].localTransform = interpolateTransform(channel, m_animationTime);
        }
    }

    computeBoneMatrices();
}

void Model::computeBoneMatrices()
{
    // Compute global transforms
    std::vector<glm::mat4> globalTransforms(m_joints.size());

    for (size_t i = 0; i < m_joints.size(); ++i)
    {
        const auto& joint = m_joints[i];

        if (joint.parentIndex >= 0)
        {
            globalTransforms[i] = globalTransforms[joint.parentIndex] * joint.localTransform;
        }
        else
        {
            globalTransforms[i] = joint.localTransform;
        }

        // Final bone matrix = globalTransform * inverseBindMatrix
        m_boneMatrices[i] = globalTransforms[i] * joint.inverseBindMatrix;
    }
}

void Model::bindTexture(int meshIndex, GLenum textureUnit) const
{
    if (meshIndex < 0 || meshIndex >= static_cast<int>(m_meshes.size()))
        return;

    int texIndex = m_meshes[meshIndex].textureIndex;
    if (texIndex >= 0 && texIndex < static_cast<int>(m_textures.size()))
    {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, m_textures[texIndex]);
    }
}

void Model::draw() const
{
    for (size_t i = 0; i < m_meshes.size(); ++i)
    {
        const auto& mesh = m_meshes[i];

        // Bind texture if available
        if (mesh.textureIndex >= 0 && mesh.textureIndex < static_cast<int>(m_textures.size()))
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_textures[mesh.textureIndex]);
        }

        glBindVertexArray(mesh.vao);

        if (mesh.ebo)
        {
            glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
        }

        glBindVertexArray(0);
    }
}
