#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include "AssetLoader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <algorithm>
#include <map>

namespace {

size_t getComponentByteSize(int componentType) {
    switch (componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: return 1;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
        case TINYGLTF_COMPONENT_TYPE_INT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        case TINYGLTF_COMPONENT_TYPE_FLOAT: return 4;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE: return 8;
        default: return 0;
    }
}

int getNumComponents(int type) {
    switch (type) {
        case TINYGLTF_TYPE_SCALAR: return 1;
        case TINYGLTF_TYPE_VEC2:   return 2;
        case TINYGLTF_TYPE_VEC3:   return 3;
        case TINYGLTF_TYPE_VEC4:   return 4;
        case TINYGLTF_TYPE_MAT2:   return 4;
        case TINYGLTF_TYPE_MAT3:   return 9;
        case TINYGLTF_TYPE_MAT4:   return 16;
        default: return 0;
    }
}

const unsigned char* getAccessorData(const tinygltf::Model& model, int accessorIndex, size_t& outCount, size_t& outStride) {
    const auto& accessor = model.accessors[accessorIndex];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    outCount = accessor.count;
    outStride = bufferView.byteStride;
    if (outStride == 0)
        outStride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

    return &buffer.data[bufferView.byteOffset + accessor.byteOffset];
}

glm::mat4 getNodeTransform(const tinygltf::Node& node) {
    if (!node.matrix.empty()) {
        return glm::make_mat4(node.matrix.data());
    }

    glm::mat4 T = glm::mat4(1.0f);
    glm::mat4 R = glm::mat4(1.0f);
    glm::mat4 S = glm::mat4(1.0f);

    if (!node.translation.empty()) {
        T = glm::translate(glm::mat4(1.0f), glm::vec3(
            (float)node.translation[0], (float)node.translation[1], (float)node.translation[2]));
    }
    if (!node.rotation.empty()) {
        glm::quat q((float)node.rotation[3], (float)node.rotation[0],
                    (float)node.rotation[1], (float)node.rotation[2]);
        R = glm::toMat4(q);
    }
    if (!node.scale.empty()) {
        S = glm::scale(glm::mat4(1.0f), glm::vec3(
            (float)node.scale[0], (float)node.scale[1], (float)node.scale[2]));
    }
    return T * R * S;
}

std::vector<GLuint> loadTextures(const tinygltf::Model& gltfModel) {
    std::vector<GLuint> textures;
    for (const auto& image : gltfModel.images) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLenum format = (image.component == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.image.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        textures.push_back(textureID);
    }
    return textures;
}

Skeleton loadSkeleton(const tinygltf::Model& gltfModel, std::map<int, int>& nodeToJoint) {
    Skeleton skeleton;
    if (gltfModel.skins.empty()) return skeleton;

    const auto& skin = gltfModel.skins[0];
    skeleton.resize(skin.joints.size());

    for (size_t i = 0; i < skin.joints.size(); ++i) {
        nodeToJoint[skin.joints[i]] = static_cast<int>(i);
    }

    if (skin.inverseBindMatrices >= 0) {
        size_t count, stride;
        const unsigned char* data = getAccessorData(gltfModel, skin.inverseBindMatrices, count, stride);
        for (size_t i = 0; i < count && i < skeleton.joints.size(); ++i) {
            const float* matData = reinterpret_cast<const float*>(data + i * stride);
            skeleton.joints[i].inverseBindMatrix = glm::make_mat4(matData);
        }
    }

    for (size_t i = 0; i < skin.joints.size(); ++i) {
        int nodeIndex = skin.joints[i];
        const auto& node = gltfModel.nodes[nodeIndex];
        glm::mat4 nodeTransform = getNodeTransform(node);
        skeleton.joints[i].localTransform = nodeTransform;
        skeleton.bindPoseTransforms[i] = nodeTransform;  // Store bind pose
        skeleton.joints[i].parentIndex = -1;

        for (size_t j = 0; j < skin.joints.size(); ++j) {
            int parentNodeIndex = skin.joints[j];
            const auto& parentNode = gltfModel.nodes[parentNodeIndex];
            for (int childIdx : parentNode.children) {
                if (childIdx == nodeIndex) {
                    skeleton.joints[i].parentIndex = static_cast<int>(j);
                    break;
                }
            }
        }
    }

    std::cout << "  Loaded skeleton with " << skeleton.joints.size() << " joints" << std::endl;
    return skeleton;
}

std::vector<AnimationClip> loadAnimations(const tinygltf::Model& gltfModel, const std::map<int, int>& nodeToJoint) {
    std::vector<AnimationClip> clips;

    for (const auto& gltfAnim : gltfModel.animations) {
        AnimationClip clip;
        clip.name = gltfAnim.name;

        for (const auto& channel : gltfAnim.channels) {
            int targetNode = channel.target_node;
            auto it = nodeToJoint.find(targetNode);
            if (it == nodeToJoint.end()) continue;

            int jointIndex = it->second;

            AnimationChannel* animChannel = nullptr;
            for (auto& ch : clip.channels) {
                if (ch.jointIndex == jointIndex) {
                    animChannel = &ch;
                    break;
                }
            }
            if (!animChannel) {
                clip.channels.push_back({});
                animChannel = &clip.channels.back();
                animChannel->jointIndex = jointIndex;
            }

            const auto& sampler = gltfAnim.samplers[channel.sampler];

            size_t timeCount, timeStride;
            const unsigned char* timeData = getAccessorData(gltfModel, sampler.input, timeCount, timeStride);
            std::vector<float> times(timeCount);
            for (size_t i = 0; i < timeCount; ++i) {
                times[i] = *reinterpret_cast<const float*>(timeData + i * timeStride);
                clip.duration = std::max(clip.duration, times[i]);
            }

            size_t count, stride;
            const unsigned char* data = getAccessorData(gltfModel, sampler.output, count, stride);

            if (channel.target_path == "translation") {
                animChannel->translationTimes = times;
                animChannel->translations.resize(count);
                for (size_t i = 0; i < count; ++i) {
                    const float* v = reinterpret_cast<const float*>(data + i * stride);
                    animChannel->translations[i] = glm::vec3(v[0], v[1], v[2]);
                }
            } else if (channel.target_path == "rotation") {
                animChannel->rotationTimes = times;
                animChannel->rotations.resize(count);
                for (size_t i = 0; i < count; ++i) {
                    const float* v = reinterpret_cast<const float*>(data + i * stride);
                    animChannel->rotations[i] = glm::quat(v[3], v[0], v[1], v[2]);
                }
            } else if (channel.target_path == "scale") {
                animChannel->scaleTimes = times;
                animChannel->scales.resize(count);
                for (size_t i = 0; i < count; ++i) {
                    const float* v = reinterpret_cast<const float*>(data + i * stride);
                    animChannel->scales[i] = glm::vec3(v[0], v[1], v[2]);
                }
            }
        }

        std::cout << "  Animation '" << clip.name << "' duration: " << clip.duration << "s" << std::endl;
        clips.push_back(std::move(clip));
    }
    return clips;
}

MeshGroup loadMeshes(const tinygltf::Model& gltfModel, const std::vector<GLuint>& textures) {
    MeshGroup group;

    for (const auto& gltfMesh : gltfModel.meshes) {
        for (const auto& primitive : gltfMesh.primitives) {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) continue;

            Mesh mesh;
            glGenVertexArrays(1, &mesh.vao);
            glBindVertexArray(mesh.vao);

            // Positions
            {
                auto it = primitive.attributes.find("POSITION");
                if (it == primitive.attributes.end()) continue;

                const auto& accessor = gltfModel.accessors[it->second];
                const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                const auto& buffer = gltfModel.buffers[bufferView.buffer];

                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t dataSize = bufferView.byteLength;
                int stride = bufferView.byteStride;
                if (stride == 0) stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                GLuint vbo;
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                glEnableVertexAttribArray(0);
            }

            // Normals
            {
                auto it = primitive.attributes.find("NORMAL");
                if (it != primitive.attributes.end()) {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    size_t dataSize = bufferView.byteLength;
                    int stride = bufferView.byteStride;
                    if (stride == 0) stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                    GLuint vbo;
                    glGenBuffers(1, &vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);
                    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
                    glEnableVertexAttribArray(1);
                }
            }

            // Texture coordinates
            {
                auto it = primitive.attributes.find("TEXCOORD_0");
                if (it != primitive.attributes.end()) {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    size_t dataSize = bufferView.byteLength;
                    int stride = bufferView.byteStride;
                    if (stride == 0) stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                    GLuint vbo;
                    glGenBuffers(1, &vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);
                    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
                    glEnableVertexAttribArray(2);
                }
            }

            // Joint indices
            {
                auto it = primitive.attributes.find("JOINTS_0");
                if (it != primitive.attributes.end()) {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    int stride = bufferView.byteStride;
                    size_t elemSize = getComponentByteSize(accessor.componentType);
                    if (stride == 0) stride = 4 * elemSize;

                    std::vector<float> jointData(accessor.count * 4);
                    for (size_t i = 0; i < accessor.count; ++i) {
                        const unsigned char* ptr = dataPtr + i * stride;
                        for (int j = 0; j < 4; ++j) {
                            if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                                jointData[i * 4 + j] = static_cast<float>(ptr[j]);
                            else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                                jointData[i * 4 + j] = static_cast<float>(reinterpret_cast<const unsigned short*>(ptr)[j]);
                        }
                    }

                    GLuint vbo;
                    glGenBuffers(1, &vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glBufferData(GL_ARRAY_BUFFER, jointData.size() * sizeof(float), jointData.data(), GL_STATIC_DRAW);
                    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
                    glEnableVertexAttribArray(3);

                    mesh.hasSkinning = true;
                }
            }

            // Joint weights
            {
                auto it = primitive.attributes.find("WEIGHTS_0");
                if (it != primitive.attributes.end()) {
                    const auto& accessor = gltfModel.accessors[it->second];
                    const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                    const auto& buffer = gltfModel.buffers[bufferView.buffer];

                    const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                    size_t dataSize = bufferView.byteLength;
                    int stride = bufferView.byteStride;
                    if (stride == 0) stride = getNumComponents(accessor.type) * getComponentByteSize(accessor.componentType);

                    GLuint vbo;
                    glGenBuffers(1, &vbo);
                    glBindBuffer(GL_ARRAY_BUFFER, vbo);
                    glBufferData(GL_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);
                    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
                    glEnableVertexAttribArray(4);
                }
            }

            // Indices
            if (primitive.indices >= 0) {
                const auto& accessor = gltfModel.accessors[primitive.indices];
                const auto& bufferView = gltfModel.bufferViews[accessor.bufferView];
                const auto& buffer = gltfModel.buffers[bufferView.buffer];

                mesh.indexCount = static_cast<GLsizei>(accessor.count);
                const unsigned char* dataPtr = &buffer.data[bufferView.byteOffset + accessor.byteOffset];
                size_t elementSize = getComponentByteSize(accessor.componentType);
                size_t dataSize = accessor.count * elementSize;

                GLuint ebo;
                glGenBuffers(1, &ebo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, dataSize, dataPtr, GL_STATIC_DRAW);

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                    mesh.indexType = GL_UNSIGNED_INT;
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                    mesh.indexType = GL_UNSIGNED_SHORT;
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                    mesh.indexType = GL_UNSIGNED_BYTE;
            }

            // Texture
            if (primitive.material >= 0) {
                const auto& material = gltfModel.materials[primitive.material];
                int texIndex = material.pbrMetallicRoughness.baseColorTexture.index;
                if (texIndex >= 0 && texIndex < static_cast<int>(gltfModel.textures.size())) {
                    int imageIndex = gltfModel.textures[texIndex].source;
                    if (imageIndex >= 0 && imageIndex < static_cast<int>(textures.size())) {
                        mesh.texture = textures[imageIndex];
                    }
                }
            }

            glBindVertexArray(0);
            group.meshes.push_back(mesh);
        }
    }

    std::cout << "  Total meshes loaded: " << group.meshes.size() << std::endl;
    return group;
}

} // anonymous namespace

LoadedModel loadGLB(const std::string& path) {
    LoadedModel result;

    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool success = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, path);

    if (!warn.empty()) std::cerr << "GLTF Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "GLTF Error: " << err << std::endl;
    if (!success) {
        std::cerr << "Failed to load GLB: " << path << std::endl;
        return result;
    }

    std::cout << "Loaded GLB: " << path << std::endl;
    std::cout << "  Meshes: " << gltfModel.meshes.size() << std::endl;
    std::cout << "  Textures: " << gltfModel.textures.size() << std::endl;
    std::cout << "  Animations: " << gltfModel.animations.size() << std::endl;
    std::cout << "  Skins: " << gltfModel.skins.size() << std::endl;

    result.textures = loadTextures(gltfModel);

    std::map<int, int> nodeToJoint;
    Skeleton skeleton = loadSkeleton(gltfModel, nodeToJoint);
    if (!skeleton.joints.empty()) {
        result.skeleton = std::move(skeleton);
    }

    result.clips = loadAnimations(gltfModel, nodeToJoint);
    result.meshGroup = loadMeshes(gltfModel, result.textures);

    return result;
}
