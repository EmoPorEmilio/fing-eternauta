#include "GLTFModel.h"
#include "tiny_gltf.h"
#include <iostream>
#include <algorithm>
#include <functional>

void DrawablePrimitive::cleanup()
{
    if (vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo[0] != 0)
    {
        glDeleteBuffers(5, vbo);
        vbo[0] = vbo[1] = vbo[2] = vbo[3] = vbo[4] = 0;
    }
    if (ebo != 0)
    {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    if (baseColorTexture != 0)
    {
        glDeleteTextures(1, &baseColorTexture);
        baseColorTexture = 0;
    }
    if (metallicRoughnessTexture != 0)
    {
        glDeleteTextures(1, &metallicRoughnessTexture);
        metallicRoughnessTexture = 0;
    }
    if (occlusionTexture != 0)
    {
        glDeleteTextures(1, &occlusionTexture);
        occlusionTexture = 0;
    }
    isInitialized = false;
}

GLTFModel::GLTFModel()
    : m_transform(1.0f), m_isLoaded(false), m_minBounds(std::numeric_limits<float>::max()), m_maxBounds(std::numeric_limits<float>::lowest())
{
}

GLTFModel::~GLTFModel()
{
    cleanup();
}

bool GLTFModel::loadFromFile(const std::string &filepath)
{
    cleanup();

    std::cout << "Loading GLTF model: " << filepath << std::endl;

    if (!loadGLB(filepath))
    {
        std::cerr << "Failed to load GLB file: " << filepath << std::endl;
        return false;
    }

    calculateBounds();
    m_isLoaded = true;

    std::cout << "Successfully loaded GLTF model with " << m_primitives.size()
              << " primitives, " << getVertexCount() << " vertices, "
              << getTriangleCount() << " triangles" << std::endl;

    return true;
}

void GLTFModel::cleanup()
{
    for (auto &primitive : m_primitives)
    {
        primitive.cleanup();
    }
    m_primitives.clear();
    m_isLoaded = false;
}

bool GLTFModel::loadGLB(const std::string &filepath)
{
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, filepath.c_str());

    if (!warn.empty())
    {
        std::cout << "GLTF Warning: " << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cerr << "GLTF Error: " << err << std::endl;
    }

    if (!ret)
    {
        std::cerr << "Failed to parse glTF file: " << filepath << std::endl;
        return false;
    }

    // Print concise model summary
    std::cout << "[GLTF] Loaded: " << model.nodes.size() << " nodes, "
              << model.meshes.size() << " meshes, "
              << model.skins.size() << " skins, "
              << model.animations.size() << " animations" << std::endl;

    // DEBUG: Print full node hierarchy
    std::cout << "[GLTF HIERARCHY]" << std::endl;
    for (size_t i = 0; i < model.nodes.size(); ++i) {
        const auto& node = model.nodes[i];
        std::cout << "  Node " << i << ": \"" << node.name << "\"";
        if (node.mesh >= 0) std::cout << " [MESH " << node.mesh << "]";
        if (node.skin >= 0) std::cout << " [SKIN " << node.skin << "]";
        if (!node.children.empty()) {
            std::cout << " children=[";
            for (size_t c = 0; c < node.children.size(); ++c) {
                if (c > 0) std::cout << ",";
                std::cout << node.children[c];
            }
            std::cout << "]";
        }
        // Print transform if not identity
        if (!node.translation.empty() || !node.rotation.empty() || !node.scale.empty() || !node.matrix.empty()) {
            std::cout << " HAS_TRANSFORM";
            if (!node.translation.empty())
                std::cout << " T=(" << node.translation[0] << "," << node.translation[1] << "," << node.translation[2] << ")";
            if (!node.scale.empty() && (node.scale[0] != 1.0 || node.scale[1] != 1.0 || node.scale[2] != 1.0))
                std::cout << " S=(" << node.scale[0] << "," << node.scale[1] << "," << node.scale[2] << ")";
        }
        std::cout << std::endl;
    }
    // Print scene roots
    if (!model.scenes.empty()) {
        std::cout << "[GLTF] Scene 0 roots: ";
        for (size_t i = 0; i < model.scenes[0].nodes.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << model.scenes[0].nodes[i];
        }
        std::cout << std::endl;
    }
    // Record source directory for potential external texture lookup
    {
        size_t slash = filepath.find_last_of("/\\");
        std::string base = (slash == std::string::npos) ? std::string("") : filepath.substr(0, slash + 1);
        // assets/models/ path -> textures sibling
        // m_sourceDir/m_texturesDir exist only conceptually (header fields were not added to keep minimal impact)
        (void)base; // placeholder in case we decide to implement external lookup
    }

    // Build skins (if any)
    m_skins.clear();
    m_skins.reserve(model.skins.size());
    for (const auto &skin : model.skins)
    {
        SkinData sd;
        sd.joints = skin.joints; // node indices
        // Inverse bind matrices
        if (skin.inverseBindMatrices >= 0)
        {
            const auto &acc = model.accessors[skin.inverseBindMatrices];
            const auto &bv = model.bufferViews[acc.bufferView];
            const auto &buf = model.buffers[bv.buffer];
            const unsigned char *base = &buf.data[bv.byteOffset + acc.byteOffset];
            size_t stride = bv.byteStride ? bv.byteStride : sizeof(float) * 16;
            sd.inverseBind.resize(acc.count);
            for (size_t i = 0; i < acc.count; ++i)
            {
                const float *m = reinterpret_cast<const float *>(base + i * stride);
                sd.inverseBind[i] = glm::mat4(
                    m[0], m[1], m[2], m[3],
                    m[4], m[5], m[6], m[7],
                    m[8], m[9], m[10], m[11],
                    m[12], m[13], m[14], m[15]);
            }
        }
        else
        {
            sd.inverseBind.assign(skin.joints.size(), glm::mat4(1.0f));
        }
        m_skins.push_back(std::move(sd));
    }

    // Precompute node global transforms in bind pose
    m_nodeGlobalTransforms.assign(model.nodes.size(), glm::mat4(1.0f));
    m_nodeBaseT.assign(model.nodes.size(), glm::vec3(0.0f));
    m_nodeBaseR.assign(model.nodes.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    m_nodeBaseS.assign(model.nodes.size(), glm::vec3(1.0f));

    // Build node hierarchy
    m_nodeParents.assign(model.nodes.size(), -1);
    m_nodeChildren.resize(model.nodes.size());
    for (size_t i = 0; i < model.nodes.size(); ++i)
    {
        m_nodeChildren[i].clear();
        for (int childIdx : model.nodes[i].children)
        {
            if (childIdx >= 0 && childIdx < (int)model.nodes.size())
            {
                m_nodeChildren[i].push_back(childIdx);
                m_nodeParents[childIdx] = (int)i;
            }
        }
    }
    // Store scene roots
    m_sceneRoots.clear();
    if (!model.scenes.empty())
    {
        for (int rootIdx : model.scenes[0].nodes)
        {
            m_sceneRoots.push_back(rootIdx);
        }
    }

    auto getLocal = [&](int nodeIndex)
    {
        const auto &n = model.nodes[nodeIndex];
        glm::mat4 local(1.0f);
        if (!n.matrix.empty())
        {
            local = glm::mat4(
                n.matrix[0], n.matrix[1], n.matrix[2], n.matrix[3],
                n.matrix[4], n.matrix[5], n.matrix[6], n.matrix[7],
                n.matrix[8], n.matrix[9], n.matrix[10], n.matrix[11],
                n.matrix[12], n.matrix[13], n.matrix[14], n.matrix[15]);
            // Not decomposing matrix into base TRS in this path
        }
        else
        {
            if (!n.translation.empty())
            {
                glm::vec3 t(
                    static_cast<float>(n.translation[0]),
                    static_cast<float>(n.translation[1]),
                    static_cast<float>(n.translation[2]));
                m_nodeBaseT[nodeIndex] = t;
                local = glm::translate(local, t);
            }
            if (!n.rotation.empty())
            {
                glm::quat r(
                    static_cast<float>(n.rotation[3]),
                    static_cast<float>(n.rotation[0]),
                    static_cast<float>(n.rotation[1]),
                    static_cast<float>(n.rotation[2]));
                m_nodeBaseR[nodeIndex] = r;
                local *= glm::mat4_cast(r);
            }
            if (!n.scale.empty())
            {
                glm::vec3 s(
                    static_cast<float>(n.scale[0]),
                    static_cast<float>(n.scale[1]),
                    static_cast<float>(n.scale[2]));
                m_nodeBaseS[nodeIndex] = s;
                local = glm::scale(local, s);
            }
        }
        return local;
    };
    std::function<void(int, const glm::mat4 &)> dfs;
    dfs = [&](int nodeIdx, const glm::mat4 &parent)
    {
        glm::mat4 local = getLocal(nodeIdx);
        glm::mat4 global = parent * local;
        m_nodeGlobalTransforms[nodeIdx] = global;
        for (int child : model.nodes[nodeIdx].children)
            dfs(child, global);
    };
    if (!model.scenes.empty())
    {
        const auto &sc = model.scenes[0];
        for (int root : sc.nodes)
            dfs(root, glm::mat4(1.0f));
    }

    // Process all nodes starting from scene root
    if (model.scenes.empty())
    {
        std::cerr << "No scenes found in GLTF model" << std::endl;
        return false;
    }

    const auto &scene = model.scenes[0];
    for (int nodeIndex : scene.nodes)
    {
        processNode(model, nodeIndex, glm::mat4(1.0f));
    }

    // Load first animation clip (MVP)
    m_animations.clear();
    if (!model.animations.empty())
    {
        const auto &anim = model.animations[0];
        AnimationClip clip;
        // Samplers
        clip.samplers.reserve(anim.samplers.size());
        for (const auto &s : anim.samplers)
        {
            AnimSampler as;
            as.interpolation = s.interpolation;
            // input times
            const auto &inAcc = model.accessors[s.input];
            const auto &inBV = model.bufferViews[inAcc.bufferView];
            const auto &inBuf = model.buffers[inBV.buffer];
            const float *tin = reinterpret_cast<const float *>(&inBuf.data[inBV.byteOffset + inAcc.byteOffset]);
            as.input.assign(tin, tin + inAcc.count);
            if (!as.input.empty())
                clip.duration = std::max(clip.duration, as.input.back());

            // output values
            const auto &outAcc = model.accessors[s.output];
            const auto &outBV = model.bufferViews[outAcc.bufferView];
            const auto &outBuf = model.buffers[outBV.buffer];
            const unsigned char *base = &outBuf.data[outBV.byteOffset + outAcc.byteOffset];
            size_t stride = outBV.byteStride ? outBV.byteStride : (outAcc.type == TINYGLTF_TYPE_VEC3 ? sizeof(float) * 3 : sizeof(float) * 4);
            int comps = (outAcc.type == TINYGLTF_TYPE_VEC3) ? 3 : 4;
            as.components = comps;
            as.output.resize(outAcc.count);
            for (size_t i = 0; i < outAcc.count; ++i)
            {
                const float *v = reinterpret_cast<const float *>(base + i * stride);
                if (comps == 3)
                    as.output[i] = glm::vec4(v[0], v[1], v[2], 0.0f);
                else
                    as.output[i] = glm::vec4(v[0], v[1], v[2], v[3]);
            }
            clip.samplers.push_back(std::move(as));
        }
        // Channels
        clip.channels.reserve(anim.channels.size());
        for (const auto &c : anim.channels)
        {
            AnimChannel ch;
            ch.samplerIndex = c.sampler;
            ch.targetNode = c.target_node;
            if (c.target_path == "translation")
                ch.path = PATH_T;
            else if (c.target_path == "rotation")
                ch.path = PATH_R;
            else
                ch.path = PATH_S; // "scale"
            clip.channels.push_back(ch);
        }
        m_animations.push_back(std::move(clip));
        m_animEnabled = true;
        std::cout << "Loaded animation clip: duration=" << m_animations[0].duration << "s, samplers=" << m_animations[0].samplers.size() << ", channels=" << m_animations[0].channels.size() << std::endl;
    }

    return true;
}

void GLTFModel::processNode(const tinygltf::Model &model, int nodeIndex, const glm::mat4 &parentTransform)
{
    if (nodeIndex < 0 || nodeIndex >= model.nodes.size())
    {
        return;
    }

    const auto &node = model.nodes[nodeIndex];
    glm::mat4 localTransform = parentTransform;

    // Apply node transformation
    if (!node.matrix.empty())
    {
        // Use explicit matrix
        glm::mat4 matrix(
            node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
            node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
            node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
            node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
        localTransform = parentTransform * matrix;
    }
    else
    {
        // Use TRS (Translation, Rotation, Scale)
        if (!node.translation.empty())
        {
            localTransform = glm::translate(localTransform, glm::vec3(
                                                                static_cast<float>(node.translation[0]),
                                                                static_cast<float>(node.translation[1]),
                                                                static_cast<float>(node.translation[2])));
        }

        if (!node.rotation.empty())
        {
            glm::quat rotation(
                static_cast<float>(node.rotation[3]),
                static_cast<float>(node.rotation[0]),
                static_cast<float>(node.rotation[1]),
                static_cast<float>(node.rotation[2]));
            localTransform = localTransform * glm::mat4_cast(rotation);
        }

        if (!node.scale.empty())
        {
            localTransform = glm::scale(localTransform, glm::vec3(
                                                            static_cast<float>(node.scale[0]),
                                                            static_cast<float>(node.scale[1]),
                                                            static_cast<float>(node.scale[2])));
        }
    }

    // Process mesh if present
    if (node.mesh >= 0)
    {
        int skinIndex = node.skin >= 0 ? node.skin : -1;
        processMesh(model, node.mesh, localTransform, skinIndex);
    }

    // Process children
    for (int childIndex : node.children)
    {
        processNode(model, childIndex, localTransform);
    }
}

void GLTFModel::processMesh(const tinygltf::Model &model, int meshIndex, const glm::mat4 &transform, int skinIndex)
{
    if (meshIndex < 0 || meshIndex >= model.meshes.size())
    {
        return;
    }

    const auto &mesh = model.meshes[meshIndex];

    for (size_t i = 0; i < mesh.primitives.size(); ++i)
    {
        const auto &primitive = mesh.primitives[i];
        processPrimitive(model, primitive, transform, primitive.material, skinIndex);
    }
}

void GLTFModel::processPrimitive(const tinygltf::Model &model, const tinygltf::Primitive &primitive,
                                 const glm::mat4 &transform, int materialIndex, int skinIndex)
{
    DrawablePrimitive drawable;

    // Set rendering mode
    drawable.glMode = (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) ? GL_TRIANGLE_STRIP : (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) ? GL_TRIANGLE_FAN
                                                                                                                                            : GL_TRIANGLES;

    // Process POSITION attribute
    auto posIt = primitive.attributes.find("POSITION");
    if (posIt == primitive.attributes.end())
    {
        std::cerr << "Primitive missing POSITION attribute" << std::endl;
        return;
    }

    const auto &posAccessor = model.accessors[posIt->second];
    const auto &posBufferView = model.bufferViews[posAccessor.bufferView];
    const auto &posBuffer = model.buffers[posBufferView.buffer];

    const unsigned char *posData = &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
    size_t posStride = posBufferView.byteStride ? posBufferView.byteStride : 3 * sizeof(float);

    drawable.vertices.reserve(posAccessor.count * 3);

    // Debug: Check first few raw vertex values for skinned meshes
    if (skinIndex >= 0 && posAccessor.count > 0)
    {
        std::cout << "[VERTEX DEBUG] Skinned mesh raw data:" << std::endl;
        std::cout << "  accessor.bufferView=" << posAccessor.bufferView
                  << ", accessor.byteOffset=" << posAccessor.byteOffset
                  << ", accessor.count=" << posAccessor.count << std::endl;
        std::cout << "  bufferView.buffer=" << posBufferView.buffer
                  << ", bufferView.byteOffset=" << posBufferView.byteOffset
                  << ", bufferView.byteLength=" << posBufferView.byteLength
                  << ", bufferView.byteStride=" << posBufferView.byteStride << std::endl;
        std::cout << "  buffer.data.size()=" << posBuffer.data.size()
                  << ", computed stride=" << posStride << std::endl;

        // Print raw bytes at buffer start to see if there's data
        size_t dataOffset = posBufferView.byteOffset + posAccessor.byteOffset;
        std::cout << "  Data offset in buffer: " << dataOffset << std::endl;
        std::cout << "  First 48 raw bytes: ";
        for (size_t i = 0; i < std::min((size_t)48, posBuffer.data.size() - dataOffset); ++i)
        {
            printf("%02X ", posBuffer.data[dataOffset + i]);
        }
        std::cout << std::endl;

        for (size_t i = 0; i < std::min((size_t)5, posAccessor.count); ++i)
        {
            const float *pos = reinterpret_cast<const float *>(posData + i * posStride);
            printf("[VERTEX DEBUG] Raw vertex %zu: (%.8f, %.8f, %.8f)\n", i, pos[0], pos[1], pos[2]);
        }
    }

    for (size_t i = 0; i < posAccessor.count; ++i)
    {
        const float *pos = reinterpret_cast<const float *>(posData + i * posStride);
        if (skinIndex >= 0)
        {
            // Keep in mesh space for skinned primitives
            drawable.vertices.insert(drawable.vertices.end(), {pos[0], pos[1], pos[2]});
        }
        else
        {
            glm::vec4 worldPos = transform * glm::vec4(pos[0], pos[1], pos[2], 1.0f);
            drawable.vertices.insert(drawable.vertices.end(), {worldPos.x, worldPos.y, worldPos.z});
        }
    }

    // Process NORMAL attribute
    auto normalIt = primitive.attributes.find("NORMAL");
    if (normalIt != primitive.attributes.end())
    {
        const auto &normalAccessor = model.accessors[normalIt->second];
        const auto &normalBufferView = model.bufferViews[normalAccessor.bufferView];
        const auto &normalBuffer = model.buffers[normalBufferView.buffer];

        const unsigned char *normalData = &normalBuffer.data[normalBufferView.byteOffset + normalAccessor.byteOffset];
        size_t normalStride = normalBufferView.byteStride ? normalBufferView.byteStride : 3 * sizeof(float);

        drawable.normals.reserve(normalAccessor.count * 3);
        if (skinIndex >= 0)
        {
            // Keep normals in mesh space; skinning shader will transform
            for (size_t i = 0; i < normalAccessor.count; ++i)
            {
                const float *normal = reinterpret_cast<const float *>(normalData + i * normalStride);
                drawable.normals.insert(drawable.normals.end(), {normal[0], normal[1], normal[2]});
            }
        }
        else
        {
            // Transform normals by inverse-transpose of upper-left 3x3
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            for (size_t i = 0; i < normalAccessor.count; ++i)
            {
                const float *normal = reinterpret_cast<const float *>(normalData + i * normalStride);
                glm::vec3 worldNormal = glm::normalize(normalMatrix * glm::vec3(normal[0], normal[1], normal[2]));
                drawable.normals.insert(drawable.normals.end(), {worldNormal.x, worldNormal.y, worldNormal.z});
            }
        }
    }

    // Debug: Print all available attributes
    std::cout << "[PRIMITIVE] Available attributes: ";
    for (const auto& attr : primitive.attributes) {
        std::cout << attr.first << " ";
    }
    std::cout << std::endl;

    // Process TEXCOORD_0 attribute
    auto texCoordIt = primitive.attributes.find("TEXCOORD_0");
    if (texCoordIt != primitive.attributes.end())
    {
        const auto &texCoordAccessor = model.accessors[texCoordIt->second];
        const auto &texCoordBufferView = model.bufferViews[texCoordAccessor.bufferView];
        const auto &texCoordBuffer = model.buffers[texCoordBufferView.buffer];

        const unsigned char *texCoordData = &texCoordBuffer.data[texCoordBufferView.byteOffset + texCoordAccessor.byteOffset];

        drawable.uvs.reserve(texCoordAccessor.count * 2);

        if (texCoordAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            size_t texCoordStride = texCoordBufferView.byteStride ? texCoordBufferView.byteStride : 2 * sizeof(float);
            for (size_t i = 0; i < texCoordAccessor.count; ++i)
            {
                const float *uv = reinterpret_cast<const float *>(texCoordData + i * texCoordStride);
                drawable.uvs.insert(drawable.uvs.end(), {uv[0], uv[1]});
            }
        } else if (texCoordAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            // Normalized unsigned short: 0-65535 maps to 0.0-1.0
            size_t texCoordStride = texCoordBufferView.byteStride ? texCoordBufferView.byteStride : 2 * sizeof(unsigned short);
            for (size_t i = 0; i < texCoordAccessor.count; ++i)
            {
                const unsigned short *uv = reinterpret_cast<const unsigned short *>(texCoordData + i * texCoordStride);
                float u = static_cast<float>(uv[0]) / 65535.0f;
                float v = static_cast<float>(uv[1]) / 65535.0f;
                drawable.uvs.insert(drawable.uvs.end(), {u, v});
            }
        } else if (texCoordAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            // Normalized unsigned byte: 0-255 maps to 0.0-1.0
            size_t texCoordStride = texCoordBufferView.byteStride ? texCoordBufferView.byteStride : 2 * sizeof(unsigned char);
            for (size_t i = 0; i < texCoordAccessor.count; ++i)
            {
                const unsigned char *uv = texCoordData + i * texCoordStride;
                float u = static_cast<float>(uv[0]) / 255.0f;
                float v = static_cast<float>(uv[1]) / 255.0f;
                drawable.uvs.insert(drawable.uvs.end(), {u, v});
            }
        }
    }

    // Process TANGENT attribute (if present)
    auto tangentIt = primitive.attributes.find("TANGENT");
    if (tangentIt != primitive.attributes.end())
    {
        const auto &tanAccessor = model.accessors[tangentIt->second];
        const auto &tanBufferView = model.bufferViews[tanAccessor.bufferView];
        const auto &tanBuffer = model.buffers[tanBufferView.buffer];

        const unsigned char *tanData = &tanBuffer.data[tanBufferView.byteOffset + tanAccessor.byteOffset];
        size_t tanStride = tanBufferView.byteStride ? tanBufferView.byteStride : 4 * sizeof(float);

        drawable.tangents.reserve(tanAccessor.count * 4);
        for (size_t i = 0; i < tanAccessor.count; ++i)
        {
            const float *t = reinterpret_cast<const float *>(tanData + i * tanStride);
            drawable.tangents.insert(drawable.tangents.end(), {t[0], t[1], t[2], t[3]});
        }
    }

    // Process indices
    if (primitive.indices >= 0)
    {
        const auto &indexAccessor = model.accessors[primitive.indices];
        const auto &indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const auto &indexBuffer = model.buffers[indexBufferView.buffer];

        const unsigned char *indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

        drawable.indices.reserve(indexAccessor.count);
        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            const unsigned short *indices = reinterpret_cast<const unsigned short *>(indexData);
            for (size_t i = 0; i < indexAccessor.count; ++i)
            {
                drawable.indices.push_back(static_cast<unsigned int>(indices[i]));
            }
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        {
            const unsigned int *indices = reinterpret_cast<const unsigned int *>(indexData);
            for (size_t i = 0; i < indexAccessor.count; ++i)
            {
                drawable.indices.push_back(indices[i]);
            }
        }
        else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            const unsigned char *indices = reinterpret_cast<const unsigned char *>(indexData);
            for (size_t i = 0; i < indexAccessor.count; ++i)
            {
                drawable.indices.push_back(static_cast<unsigned int>(indices[i]));
            }
        }
    }

    // JOINTS_0 and WEIGHTS_0 (skinning)
    if (skinIndex >= 0)
    {
        auto jointsIt = primitive.attributes.find("JOINTS_0");
        auto weightsIt = primitive.attributes.find("WEIGHTS_0");
        if (jointsIt != primitive.attributes.end() && weightsIt != primitive.attributes.end())
        {
            const auto &jAcc = model.accessors[jointsIt->second];
            const auto &jBV = model.bufferViews[jAcc.bufferView];
            const auto &jBuf = model.buffers[jBV.buffer];
            const unsigned char *jData = &jBuf.data[jBV.byteOffset + jAcc.byteOffset];
            size_t jStride = jBV.byteStride ? jBV.byteStride : 4 * sizeof(unsigned short);

            const auto &wAcc = model.accessors[weightsIt->second];
            const auto &wBV = model.bufferViews[wAcc.bufferView];
            const auto &wBuf = model.buffers[wBV.buffer];
            const unsigned char *wData = &wBuf.data[wBV.byteOffset + wAcc.byteOffset];
            size_t wStrideDefault = 0;
            if (wAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                wStrideDefault = 4 * sizeof(float);
            else if (wAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                wStrideDefault = 4 * sizeof(unsigned short);
            else if (wAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                wStrideDefault = 4 * sizeof(unsigned char);
            size_t wStride = wBV.byteStride ? wBV.byteStride : wStrideDefault;

            drawable.jointIndices.resize(jAcc.count * 4);
            drawable.jointWeights.resize(wAcc.count * 4);


            for (size_t i = 0; i < jAcc.count; ++i)
            {
                // JOINTS can be UNSIGNED_BYTE or UNSIGNED_SHORT per glTF spec; handle both
                if (jAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    const unsigned char *ji = reinterpret_cast<const unsigned char *>(jData + i * (jBV.byteStride ? jBV.byteStride : 4));
                    drawable.jointIndices[i * 4 + 0] = ji[0];
                    drawable.jointIndices[i * 4 + 1] = ji[1];
                    drawable.jointIndices[i * 4 + 2] = ji[2];
                    drawable.jointIndices[i * 4 + 3] = ji[3];
                }
                else
                {
                    const unsigned short *ji = reinterpret_cast<const unsigned short *>(jData + i * jStride);
                    drawable.jointIndices[i * 4 + 0] = ji[0];
                    drawable.jointIndices[i * 4 + 1] = ji[1];
                    drawable.jointIndices[i * 4 + 2] = ji[2];
                    drawable.jointIndices[i * 4 + 3] = ji[3];
                }

                // WEIGHTS can be float or normalized ubyte/ushort
                if (wAcc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                {
                    const float *jw = reinterpret_cast<const float *>(wData + i * wStride);
                    drawable.jointWeights[i * 4 + 0] = jw[0];
                    drawable.jointWeights[i * 4 + 1] = jw[1];
                    drawable.jointWeights[i * 4 + 2] = jw[2];
                    drawable.jointWeights[i * 4 + 3] = jw[3];
                }
                else if (wAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    const unsigned char *jw = reinterpret_cast<const unsigned char *>(wData + i * (wBV.byteStride ? wBV.byteStride : 4));
                    drawable.jointWeights[i * 4 + 0] = (float)jw[0] / 255.0f;
                    drawable.jointWeights[i * 4 + 1] = (float)jw[1] / 255.0f;
                    drawable.jointWeights[i * 4 + 2] = (float)jw[2] / 255.0f;
                    drawable.jointWeights[i * 4 + 3] = (float)jw[3] / 255.0f;
                }
                else if (wAcc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    const unsigned short *jw = reinterpret_cast<const unsigned short *>(wData + i * wStride);
                    drawable.jointWeights[i * 4 + 0] = (float)jw[0] / 65535.0f;
                    drawable.jointWeights[i * 4 + 1] = (float)jw[1] / 65535.0f;
                    drawable.jointWeights[i * 4 + 2] = (float)jw[2] / 65535.0f;
                    drawable.jointWeights[i * 4 + 3] = (float)jw[3] / 65535.0f;
                }

                // Normalize weights to sum to 1 to avoid collapse
                float s = drawable.jointWeights[i * 4 + 0] + drawable.jointWeights[i * 4 + 1] + drawable.jointWeights[i * 4 + 2] + drawable.jointWeights[i * 4 + 3];
                if (s > 0.0f)
                {
                    drawable.jointWeights[i * 4 + 0] /= s;
                    drawable.jointWeights[i * 4 + 1] /= s;
                    drawable.jointWeights[i * 4 + 2] /= s;
                    drawable.jointWeights[i * 4 + 3] /= s;
                }

            }
            drawable.skinIndex = skinIndex;
        }
    }

    // Process material
    if (materialIndex >= 0 && materialIndex < model.materials.size())
    {
        const auto &material = model.materials[materialIndex];

        // Base color factor - initialize with default first
        drawable.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default white
        if (!material.pbrMetallicRoughness.baseColorFactor.empty())
        {
            drawable.baseColorFactor = glm::vec4(
                material.pbrMetallicRoughness.baseColorFactor[0],
                material.pbrMetallicRoughness.baseColorFactor[1],
                material.pbrMetallicRoughness.baseColorFactor[2],
                material.pbrMetallicRoughness.baseColorFactor[3]);
        }

        drawable.metallicFactor = static_cast<float>(material.pbrMetallicRoughness.metallicFactor);
        drawable.roughnessFactor = static_cast<float>(material.pbrMetallicRoughness.roughnessFactor);

        // Base color texture
        if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
        {
            // Base color is sRGB
            drawable.baseColorTexture = loadTexture(model, material.pbrMetallicRoughness.baseColorTexture.index, true);
            drawable.hasBaseColorTexture = (drawable.baseColorTexture != 0);

            // Debug: Check which texCoord set the material expects
            int texCoordSet = material.pbrMetallicRoughness.baseColorTexture.texCoord;
            std::cout << "[MATERIAL] Base color texture uses TEXCOORD_" << texCoordSet << std::endl;
            if (texCoordSet != 0) {
                std::cout << "[WARNING] Material expects TEXCOORD_" << texCoordSet << " but we only load TEXCOORD_0!" << std::endl;
            }
        }

        // Metallic-roughness texture
        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
        {
            // Metallic-roughness is linear
            drawable.metallicRoughnessTexture = loadTexture(model, material.pbrMetallicRoughness.metallicRoughnessTexture.index, false);
            drawable.hasMetallicRoughnessTexture = (drawable.metallicRoughnessTexture != 0);
        }

        // Occlusion texture
        if (material.occlusionTexture.index >= 0)
        {
            // Occlusion is linear
            drawable.occlusionTexture = loadTexture(model, material.occlusionTexture.index, false);
            drawable.hasOcclusionTexture = (drawable.occlusionTexture != 0);
            drawable.occlusionStrength = static_cast<float>(material.occlusionTexture.strength);
        }

        // Normal texture
        if (material.normalTexture.index >= 0)
        {
            drawable.normalTexture = loadTexture(model, material.normalTexture.index, false); // linear
            drawable.hasNormalTexture = (drawable.normalTexture != 0);
            drawable.normalScale = material.normalTexture.scale > 0.0 ? (float)material.normalTexture.scale : 1.0f;
        }
    }
    else
    {
        // Set default material properties
        drawable.baseColorFactor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
        drawable.metallicFactor = 0.0f;
        drawable.roughnessFactor = 0.5f;
        drawable.occlusionStrength = 1.0f;
    }

    // Set default colors if none provided
    if (drawable.colors.empty())
    {
        drawable.colors.assign(drawable.vertices.size(), 1.0f);
    }

    setupPrimitive(drawable);
    m_primitives.push_back(std::move(drawable));
}

GLuint GLTFModel::loadTexture(const tinygltf::Model &model, int textureIndex, bool srgb)
{
    if (textureIndex < 0 || textureIndex >= model.textures.size())
        return 0;

    const auto &texture = model.textures[textureIndex];
    if (texture.source < 0 || texture.source >= model.images.size())
        return 0;

    const auto &image = model.images[texture.source];

    // Debug texture info
    std::cout << "[TEXTURE] Loading texture " << textureIndex << ": "
              << image.width << "x" << image.height << ", "
              << image.component << " channels, "
              << image.image.size() << " bytes" << std::endl;
    if (!image.name.empty()) {
        std::cout << "[TEXTURE] Name: " << image.name << std::endl;
    }
    if (!image.uri.empty()) {
        std::cout << "[TEXTURE] URI: " << image.uri << std::endl;
    }

    // Get sampler info for wrap modes (default to REPEAT per glTF spec)
    GLenum wrapS = GL_REPEAT;
    GLenum wrapT = GL_REPEAT;
    if (texture.sampler >= 0 && texture.sampler < (int)model.samplers.size()) {
        const auto& sampler = model.samplers[texture.sampler];
        // glTF wrap modes: 10497=REPEAT, 33071=CLAMP_TO_EDGE, 33648=MIRRORED_REPEAT
        auto convertWrap = [](int gltfWrap) -> GLenum {
            switch (gltfWrap) {
                case 33071: return GL_CLAMP_TO_EDGE;
                case 33648: return GL_MIRRORED_REPEAT;
                case 10497:
                default: return GL_REPEAT;
            }
        };
        wrapS = convertWrap(sampler.wrapS);
        wrapT = convertWrap(sampler.wrapT);
        std::cout << "[TEXTURE] Sampler wrapS=" << sampler.wrapS << " wrapT=" << sampler.wrapT << std::endl;
    } else {
        std::cout << "[TEXTURE] No sampler, using default GL_REPEAT" << std::endl;
    }

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    GLenum internalFormat;
    if (srgb)
    {
        internalFormat = (image.component == 4) ? GL_SRGB8_ALPHA8 : GL_SRGB8;
    }
    else
    {
        internalFormat = (image.component == 4) ? GL_RGBA8 : GL_RGB8;
    }
    GLenum format = (image.component == 4) ? GL_RGBA : GL_RGB;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.width, image.height, 0, format, GL_UNSIGNED_BYTE, image.image.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}

void GLTFModel::setupPrimitive(DrawablePrimitive &primitive)
{
    if (primitive.isInitialized)
        return;

    // Generate OpenGL objects
    glGenVertexArrays(1, &primitive.vao);
    glGenBuffers(7, primitive.vbo);
    glGenBuffers(1, &primitive.ebo);

    glBindVertexArray(primitive.vao);

    // Position buffer
    glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, primitive.vertices.size() * sizeof(GLfloat), primitive.vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Normal buffer
    if (!primitive.normals.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, primitive.normals.size() * sizeof(GLfloat), primitive.normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Color buffer
    if (!primitive.colors.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, primitive.colors.size() * sizeof(GLfloat), primitive.colors.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // UV buffer
    if (!primitive.uvs.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo[3]);
        glBufferData(GL_ARRAY_BUFFER, primitive.uvs.size() * sizeof(GLfloat), primitive.uvs.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Tangent buffer (vec4)
    if (!primitive.tangents.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo[4]);
        glBufferData(GL_ARRAY_BUFFER, primitive.tangents.size() * sizeof(GLfloat), primitive.tangents.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Weights buffer (vec4)
    if (!primitive.jointWeights.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo[5]);
        glBufferData(GL_ARRAY_BUFFER, primitive.jointWeights.size() * sizeof(GLfloat), primitive.jointWeights.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    }

    // Joints buffer (uvec4) - convert from unsigned short to unsigned int for shader compatibility
    if (!primitive.jointIndices.empty())
    {
        // Convert joint indices from unsigned short to unsigned int
        std::vector<unsigned int> jointIndicesInt;
        jointIndicesInt.reserve(primitive.jointIndices.size());
        for (unsigned short joint : primitive.jointIndices)
        {
            jointIndicesInt.push_back(static_cast<unsigned int>(joint));
        }

        glBindBuffer(GL_ARRAY_BUFFER, primitive.vbo[6]);
        glBufferData(GL_ARRAY_BUFFER, jointIndicesInt.size() * sizeof(unsigned int), jointIndicesInt.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(6);
        glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, 0, nullptr);

    }

    // Index buffer
    if (!primitive.indices.empty())
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, primitive.indices.size() * sizeof(unsigned int), primitive.indices.data(), GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
    primitive.isInitialized = true;
}

void GLTFModel::calculateBounds()
{
    m_minBounds = glm::vec3(std::numeric_limits<float>::max());
    m_maxBounds = glm::vec3(std::numeric_limits<float>::lowest());

    bool foundVertices = false;
    bool isSkinnedModel = false;

    for (const auto &primitive : m_primitives)
    {
        if (primitive.skinIndex >= 0)
            isSkinnedModel = true;

        for (size_t i = 0; i < primitive.vertices.size(); i += 3)
        {
            if (i + 2 < primitive.vertices.size())
            {
                glm::vec3 vertex(primitive.vertices[i], primitive.vertices[i + 1], primitive.vertices[i + 2]);

                // For skinned models, vertices might be at origin - use default bounds
                if (isSkinnedModel && glm::length(vertex) < 0.001f)
                {
                    if (!foundVertices)
                    {
                        m_minBounds = glm::vec3(-1.0f, 0.0f, -1.0f);
                        m_maxBounds = glm::vec3(1.0f, 2.0f, 1.0f);
                        foundVertices = true;
                    }
                }
                else
                {
                    m_minBounds = glm::min(m_minBounds, vertex);
                    m_maxBounds = glm::max(m_maxBounds, vertex);
                    foundVertices = true;
                }
            }
        }
    }

    if (!foundVertices)
    {
        m_minBounds = glm::vec3(-1.0f, 0.0f, -1.0f);
        m_maxBounds = glm::vec3(1.0f, 2.0f, 1.0f);
    }
}

void GLTFModel::render(const glm::mat4 &view, const glm::mat4 &projection,
                       const glm::vec3 &cameraPos, const glm::vec3 &lightDir,
                       const glm::vec3 &lightColor, GLuint shaderProgram)
{
    if (!m_isLoaded || shaderProgram == 0)
    {
        return;
    }

    // If we have an animation clip, evaluate it and rebuild node globals
    static int animCheckCount = 0;
    if (++animCheckCount % 60 == 0) {
        std::cout << "[ANIM CHECK] enabled=" << m_animEnabled << ", animations=" << m_animations.size() << ", time=" << m_animTime << std::endl;
    }
    if (m_animEnabled && !m_animations.empty())
    {
        float t = m_animTime;
        const AnimationClip &clip = m_animations[0];
        if (clip.duration > 0.0f)
        {
            // Loop
            t = fmodf(t, clip.duration);
        }

        // Print animation status once per second
        static float lastDebugTime = -1.0f;
        if (m_animTime - lastDebugTime > 1.0f || m_animTime < lastDebugTime)
        {
            std::cout << "[ANIM] time=" << m_animTime << "/" << clip.duration
                      << ", looped_t=" << t
                      << ", channels=" << clip.channels.size() << std::endl;
            // Print first few channels and their target nodes
            for (size_t i = 0; i < std::min((size_t)5, clip.channels.size()); ++i)
            {
                std::cout << "[ANIM] channel[" << i << "]: targetNode=" << clip.channels[i].targetNode
                          << ", path=" << (clip.channels[i].path == PATH_T ? "T" : (clip.channels[i].path == PATH_R ? "R" : "S"))
                          << std::endl;
            }
            lastDebugTime = m_animTime;
        }
        // Start from base TRS
        // Rebuild locals per node - CRITICAL FIX: Expand to accommodate all animation target nodes
        // Find the maximum target node index from animation channels
        int maxTargetNode = (int)m_nodeGlobalTransforms.size() - 1;
        for (const auto &ch : clip.channels)
        {
            if (ch.targetNode > maxTargetNode)
            {
                maxTargetNode = ch.targetNode;
            }
        }

        // Create locals array large enough for all target nodes
        std::vector<glm::mat4> locals(maxTargetNode + 1, glm::mat4(1.0f));

        // Initialize with base transforms for existing nodes
        for (size_t i = 0; i < m_nodeGlobalTransforms.size(); ++i)
        {
            locals[i] = glm::translate(glm::mat4(1.0f), m_nodeBaseT[i]) * glm::mat4_cast(m_nodeBaseR[i]) * glm::scale(glm::mat4(1.0f), m_nodeBaseS[i]);
        }

        // Initialize remaining nodes with identity (for nodes beyond our current node count)
        for (int i = (int)m_nodeGlobalTransforms.size(); i <= maxTargetNode; ++i)
        {
            locals[i] = glm::mat4(1.0f);
        }

        // Ensure m_nodeBaseT/R/S and hierarchy arrays are large enough for all animation target nodes
        size_t requiredSize = static_cast<size_t>(maxTargetNode + 1);
        if (m_nodeBaseT.size() < requiredSize)
        {
            m_nodeBaseT.resize(requiredSize, glm::vec3(0.0f));
            m_nodeBaseR.resize(requiredSize, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            m_nodeBaseS.resize(requiredSize, glm::vec3(1.0f));
            m_nodeParents.resize(requiredSize, -1);
            m_nodeChildren.resize(requiredSize);
        }

        // Create temporary arrays for animated T/R/S - start from base values
        std::vector<glm::vec3> animT(requiredSize);
        std::vector<glm::quat> animR(requiredSize);
        std::vector<glm::vec3> animS(requiredSize);
        for (size_t i = 0; i < requiredSize; ++i)
        {
            animT[i] = m_nodeBaseT[i];
            animR[i] = m_nodeBaseR[i];
            animS[i] = m_nodeBaseS[i];
        }

        // Apply animated channels - accumulate into T/R/S arrays, don't overwrite locals yet
        for (const auto &ch : clip.channels)
        {
            if (ch.targetNode < 0 || ch.targetNode >= (int)requiredSize)
                continue;
            const AnimSampler &samp = clip.samplers[ch.samplerIndex];
            if (samp.input.empty())
                continue;
            // Find keyframe interval
            size_t k1 = 0, k2 = 0;
            if (t <= samp.input.front())
            {
                k1 = k2 = 0;
            }
            else if (t >= samp.input.back())
            {
                k1 = k2 = samp.input.size() - 1;
            }
            else
            {
                for (size_t i = 0; i + 1 < samp.input.size(); ++i)
                {
                    if (t >= samp.input[i] && t <= samp.input[i + 1])
                    {
                        k1 = i;
                        k2 = i + 1;
                        break;
                    }
                }
            }
            float a = (k1 == k2) ? 0.0f : (t - samp.input[k1]) / (samp.input[k2] - samp.input[k1]);
            a = glm::clamp(a, 0.0f, 1.0f);
            if (ch.path == PATH_T)
            {
                glm::vec3 v0 = glm::vec3(samp.output[k1]);
                glm::vec3 v1 = glm::vec3(samp.output[k2]);
                animT[ch.targetNode] = glm::mix(v0, v1, a);
            }
            else if (ch.path == PATH_R)
            {
                glm::quat q0 = glm::quat(samp.output[k1].w, samp.output[k1].x, samp.output[k1].y, samp.output[k1].z);
                glm::quat q1 = glm::quat(samp.output[k2].w, samp.output[k2].x, samp.output[k2].y, samp.output[k2].z);
                animR[ch.targetNode] = glm::slerp(q0, q1, a);
            }
            else // PATH_S
            {
                glm::vec3 s0 = glm::vec3(samp.output[k1]);
                glm::vec3 s1 = glm::vec3(samp.output[k2]);
                animS[ch.targetNode] = glm::mix(s0, s1, a);
            }
        }

        // Now build locals from the accumulated animated T/R/S
        for (size_t i = 0; i < requiredSize; ++i)
        {
            if (i < locals.size())
            {
                locals[i] = glm::translate(glm::mat4(1.0f), animT[i]) * glm::mat4_cast(animR[i]) * glm::scale(glm::mat4(1.0f), animS[i]);
            }
        }
        // Rebuild global transforms from locals using the stored node hierarchy
        if (!locals.empty())
        {
            // Ensure global transforms array is large enough
            if (m_nodeGlobalTransforms.size() < locals.size())
            {
                m_nodeGlobalTransforms.resize(locals.size(), glm::mat4(1.0f));
            }

            // Proper DFS traversal using stored hierarchy
            std::function<void(int, const glm::mat4&)> traverseHierarchy = [&](int nodeIdx, const glm::mat4& parentGlobal)
            {
                if (nodeIdx < 0 || nodeIdx >= (int)locals.size()) return;

                // Compute global transform: parent * local
                glm::mat4 global = parentGlobal * locals[nodeIdx];
                m_nodeGlobalTransforms[nodeIdx] = global;

                // Traverse children
                if (nodeIdx < (int)m_nodeChildren.size())
                {
                    for (int childIdx : m_nodeChildren[nodeIdx])
                    {
                        traverseHierarchy(childIdx, global);
                    }
                }
            };

            // Start DFS from scene roots
            for (int rootIdx : m_sceneRoots)
            {
                traverseHierarchy(rootIdx, glm::mat4(1.0f));
            }

            // Handle any nodes not reachable from scene roots (orphaned but animated nodes)
            for (size_t i = 0; i < locals.size(); ++i)
            {
                if (m_nodeParents.size() > i && m_nodeParents[i] == -1)
                {
                    bool isSceneRoot = false;
                    for (int r : m_sceneRoots)
                    {
                        if (r == (int)i) { isSceneRoot = true; break; }
                    }
                    if (!isSceneRoot)
                    {
                        // Orphan node - use local as global
                        m_nodeGlobalTransforms[i] = locals[i];
                    }
                }
            }
        }
    }

    glUseProgram(shaderProgram);

    // Set matrices
    glm::mat4 modelMatrix = m_transform;
    glm::mat4 mvp = projection * view * modelMatrix;
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));

    // Get uniform locations
    GLint locMVP = glGetUniformLocation(shaderProgram, "uMVP");
    GLint locModel = glGetUniformLocation(shaderProgram, "uModel");
    GLint locView = glGetUniformLocation(shaderProgram, "uView");
    GLint locProjection = glGetUniformLocation(shaderProgram, "uProjection");
    GLint locNormalMatrix = glGetUniformLocation(shaderProgram, "uNormalMatrix");
    GLint locCameraPos = glGetUniformLocation(shaderProgram, "uCameraPos");
    GLint locLightDir = glGetUniformLocation(shaderProgram, "uLightDir");
    GLint locLightColor = glGetUniformLocation(shaderProgram, "uLightColor");

    // Set uniforms
    if (locMVP >= 0)
        glUniformMatrix4fv(locMVP, 1, GL_FALSE, glm::value_ptr(mvp));
    if (locModel >= 0)
        glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    if (locView >= 0)
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
    if (locProjection >= 0)
        glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(projection));
    if (locNormalMatrix >= 0)
        glUniformMatrix3fv(locNormalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    if (locCameraPos >= 0)
        glUniform3fv(locCameraPos, 1, glm::value_ptr(cameraPos));
    if (locLightDir >= 0)
        glUniform3fv(locLightDir, 1, glm::value_ptr(lightDir));
    if (locLightColor >= 0)
        glUniform3fv(locLightColor, 1, glm::value_ptr(lightColor));

    // Prepare skinning uniforms
    GLint locSkinned = glGetUniformLocation(shaderProgram, "uSkinned");
    GLint locJointCount = glGetUniformLocation(shaderProgram, "uJointCount");
    GLint locJointMatrices = glGetUniformLocation(shaderProgram, "uJointMatrices[0]");

    // Render each primitive
    static int primDebugCount = 0;
    for (const auto &primitive : m_primitives)
    {
        if (!primitive.isInitialized)
            continue;

        // Debug: Print primitive skinIndex once
        if (primDebugCount++ < 5)
        {
            std::cout << "[RENDER] primitive.skinIndex=" << primitive.skinIndex
                      << ", m_animEnabled=" << m_animEnabled
                      << ", m_skins.size()=" << m_skins.size() << std::endl;
        }

        // Upload joint palette if skinned AND animation is enabled
        // When animation is disabled, bypass skinning to show raw mesh vertices
        if (primitive.skinIndex >= 0 && m_animEnabled)
        {
            if (locSkinned >= 0 && locJointCount >= 0 && locJointMatrices >= 0)
            {
                const SkinData &sd = m_skins[primitive.skinIndex];
                int jointCount = (int)std::min(sd.joints.size(), (size_t)64);
                std::vector<glm::mat4> palette(jointCount);

                for (int i = 0; i < jointCount; ++i)
                {
                    int nodeIdx = sd.joints[i];
                    if (nodeIdx >= (int)m_nodeGlobalTransforms.size())
                        continue;

                    glm::mat4 jointGlobal = m_nodeGlobalTransforms[nodeIdx];
                    palette[i] = jointGlobal * sd.inverseBind[i];
                }

                glUniform1i(locSkinned, 1);
                glUniform1i(locJointCount, jointCount);
                glUniformMatrix4fv(locJointMatrices, jointCount, GL_FALSE, glm::value_ptr(palette[0]));

                // Debug: Print skinning info once per second
                static float lastSkinDebugTime = -1.0f;
                if (m_animTime - lastSkinDebugTime > 1.0f || m_animTime < lastSkinDebugTime)
                {
                    std::cout << "[SKIN] primitive.skinIndex=" << primitive.skinIndex
                              << ", jointCount=" << jointCount
                              << ", uSkinned loc=" << locSkinned
                              << ", uJointMatrices loc=" << locJointMatrices << std::endl;
                    // Print node 26 (Hips - directly animated) and node 4 (tail - also animated)
                    // to verify animation is updating global transforms
                    if (m_nodeGlobalTransforms.size() > 26)
                    {
                        std::cout << "[SKIN] node26(Hips)[3]=(" << m_nodeGlobalTransforms[26][3][0]
                                  << "," << m_nodeGlobalTransforms[26][3][1]
                                  << "," << m_nodeGlobalTransforms[26][3][2] << ")"
                                  << ", node4(tail)[3]=(" << m_nodeGlobalTransforms[4][3][0]
                                  << "," << m_nodeGlobalTransforms[4][3][1]
                                  << "," << m_nodeGlobalTransforms[4][3][2] << ")"
                                  << ", node0(tail3)[3]=(" << m_nodeGlobalTransforms[0][3][0]
                                  << "," << m_nodeGlobalTransforms[0][3][1]
                                  << "," << m_nodeGlobalTransforms[0][3][2] << ")" << std::endl;
                    }
                    lastSkinDebugTime = m_animTime;
                }
            }
            else
            {
                // Debug: Uniform locations not found
                static bool warnedOnce = false;
                if (!warnedOnce)
                {
                    std::cout << "[SKIN WARNING] Uniform locations not found! locSkinned=" << locSkinned
                              << ", locJointCount=" << locJointCount
                              << ", locJointMatrices=" << locJointMatrices << std::endl;
                    warnedOnce = true;
                }
            }
        }
        else if (locSkinned >= 0)
        {
            // Skinning disabled - use raw vertex positions
            glUniform1i(locSkinned, 0);
        }

        // Set material uniforms (remain in linear space)
        GLint locBaseColorFactor = glGetUniformLocation(shaderProgram, "uBaseColorFactor");
        GLint locMetallicFactor = glGetUniformLocation(shaderProgram, "uMetallicFactor");
        GLint locRoughnessFactor = glGetUniformLocation(shaderProgram, "uRoughnessFactor");
        GLint locOcclusionStrength = glGetUniformLocation(shaderProgram, "uOcclusionStrength");

        if (locBaseColorFactor >= 0)
            glUniform4fv(locBaseColorFactor, 1, glm::value_ptr(primitive.baseColorFactor));
        if (locMetallicFactor >= 0)
            glUniform1f(locMetallicFactor, primitive.metallicFactor);
        if (locRoughnessFactor >= 0)
            glUniform1f(locRoughnessFactor, primitive.roughnessFactor);
        if (locOcclusionStrength >= 0)
            glUniform1f(locOcclusionStrength, primitive.occlusionStrength);

        // Set texture flags
        GLint locHasBaseColorTex = glGetUniformLocation(shaderProgram, "uHasBaseColorTexture");
        GLint locHasMRTex = glGetUniformLocation(shaderProgram, "uHasMetallicRoughnessTexture");
        GLint locHasOcclusionTex = glGetUniformLocation(shaderProgram, "uHasOcclusionTexture");
        GLint locHasNormalTex = glGetUniformLocation(shaderProgram, "uHasNormalTexture");
        GLint locNormalScale = glGetUniformLocation(shaderProgram, "uNormalScale");

        if (locHasBaseColorTex >= 0)
            glUniform1i(locHasBaseColorTex, primitive.hasBaseColorTexture ? 1 : 0);
        if (locHasMRTex >= 0)
            glUniform1i(locHasMRTex, primitive.hasMetallicRoughnessTexture ? 1 : 0);
        if (locHasOcclusionTex >= 0)
            glUniform1i(locHasOcclusionTex, primitive.hasOcclusionTexture ? 1 : 0);
        if (locHasNormalTex >= 0)
            glUniform1i(locHasNormalTex, primitive.hasNormalTexture ? 1 : 0);
        if (locNormalScale >= 0)
            glUniform1f(locNormalScale, primitive.normalScale);

        // Bind textures
        if (primitive.hasBaseColorTexture)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, primitive.baseColorTexture);
            GLint locBaseColorTex = glGetUniformLocation(shaderProgram, "uBaseColorTexture");
            if (locBaseColorTex >= 0)
                glUniform1i(locBaseColorTex, 0);
        }

        if (primitive.hasMetallicRoughnessTexture)
        {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, primitive.metallicRoughnessTexture);
            GLint locMRTex = glGetUniformLocation(shaderProgram, "uMetallicRoughnessTexture");
            if (locMRTex >= 0)
                glUniform1i(locMRTex, 1);
        }

        if (primitive.hasOcclusionTexture)
        {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, primitive.occlusionTexture);
            GLint locOcclusionTex = glGetUniformLocation(shaderProgram, "uOcclusionTexture");
            if (locOcclusionTex >= 0)
                glUniform1i(locOcclusionTex, 2);
        }

        if (primitive.hasNormalTexture)
        {
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, primitive.normalTexture);
            GLint locNormalTex = glGetUniformLocation(shaderProgram, "uNormalTexture");
            if (locNormalTex >= 0)
                glUniform1i(locNormalTex, 3);
        }

        // Render
        glBindVertexArray(primitive.vao);

        if (!primitive.indices.empty())
        {
            glDrawElements(primitive.glMode, static_cast<GLsizei>(primitive.indices.size()), GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            glDrawArrays(primitive.glMode, 0, static_cast<GLsizei>(primitive.vertices.size() / 3));
        }

        glBindVertexArray(0);
    }

    glUseProgram(0);
}

size_t GLTFModel::getVertexCount() const
{
    size_t count = 0;
    for (const auto &primitive : m_primitives)
    {
        count += primitive.vertices.size() / 3;
    }
    return count;
}

size_t GLTFModel::getTriangleCount() const
{
    size_t count = 0;
    for (const auto &primitive : m_primitives)
    {
        if (!primitive.indices.empty())
        {
            count += primitive.indices.size() / 3;
        }
        else
        {
            count += primitive.vertices.size() / 9; // 3 vertices per triangle
        }
    }
    return count;
}

glm::vec3 GLTFModel::getCenter() const
{
    return (m_minBounds + m_maxBounds) * 0.5f;
}

float GLTFModel::getRadius() const
{
    glm::vec3 center = getCenter();
    float maxDistance = 0.0f;

    for (const auto &primitive : m_primitives)
    {
        for (size_t i = 0; i < primitive.vertices.size(); i += 3)
        {
            glm::vec3 vertex(primitive.vertices[i], primitive.vertices[i + 1], primitive.vertices[i + 2]);
            float distance = glm::length(vertex - center);
            maxDistance = std::max(maxDistance, distance);
        }
    }

    return maxDistance;
}
