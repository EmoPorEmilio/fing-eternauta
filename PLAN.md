# ECS Architecture Plan

## Current State

The codebase is a monolithic OpenGL character animation viewer with:
- **Model.cpp/h** - God object handling loading, animation, rendering
- **main.cpp** - Game loop with hardcoded render passes
- **Camera.h** - Camera with embedded input handling
- **Shader.h/cpp** - Shader compilation (keep as-is, works well)

---

## Target ECS Architecture

### Core Structure

```
src/
├── ecs/
│   ├── Entity.h          # Entity ID (just a uint32_t)
│   ├── Registry.h        # Entity-component storage
│   │
│   ├── components/
│   │   ├── Transform.h       # Position, rotation, scale
│   │   ├── Mesh.h            # VAO, VBO handles, index count
│   │   ├── Skeleton.h        # Joints array, bone matrices
│   │   ├── Animation.h       # Current animation state
│   │   ├── Renderable.h      # Shader type, texture ID
│   │   └── Camera.h          # FOV, near/far, active flag
│   │
│   └── systems/
│       ├── AnimationSystem.h     # Updates animation time, interpolates keyframes
│       ├── SkeletonSystem.h      # Computes bone matrices from skeleton
│       ├── InputSystem.h         # Polls SDL, updates camera transform
│       └── RenderSystem.h        # Draws all renderables
│
├── assets/
│   └── AssetLoader.h     # Load GLB → creates components (not a system)
│
├── Shader.h              # Keep existing
├── Shader.cpp            # Keep existing
└── main.cpp              # Slim: init, loop, shutdown
```

---

## Components

### 1. Transform
```cpp
struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 matrix() const;
};
```

### 2. Mesh
```cpp
struct Mesh {
    GLuint vao;
    GLsizei indexCount;
    GLenum indexType;
    bool hasSkinning;
};
```
One component per mesh. Entity can have multiple via MeshGroup or separate entities.

### 3. Skeleton
```cpp
struct Joint {
    int parentIndex;
    glm::mat4 inverseBindMatrix;
    glm::mat4 localTransform;
};

struct Skeleton {
    std::vector<Joint> joints;
    std::vector<glm::mat4> boneMatrices;  // Computed each frame
};
```

### 4. Animation
```cpp
struct AnimationClip {
    float duration;
    struct Channel {
        int jointIndex;
        std::vector<float> times;
        std::vector<glm::vec3> translations;
        std::vector<glm::quat> rotations;
        std::vector<glm::vec3> scales;
    };
    std::vector<Channel> channels;
};

struct Animation {
    AnimationClip* clip;       // Pointer to shared clip data
    float time;
    bool playing;
};
```

### 5. Renderable
```cpp
enum class ShaderType { Color, Model, Skinned };

struct Renderable {
    ShaderType shader;
    GLuint texture;   // 0 = no texture
};
```

### 6. Camera
```cpp
struct Camera {
    float fov;
    float nearPlane;
    float farPlane;
    bool active;

    glm::mat4 viewMatrix(const Transform& t) const;
    glm::mat4 projectionMatrix(float aspect) const;
};
```

---

## Systems

### 1. InputSystem
```cpp
void InputSystem::update(Registry& reg, float dt) {
    // Poll SDL events
    // Find entity with Camera + Transform
    // Update transform based on WASD/mouse
}
```
- Reads: SDL events
- Writes: Transform (of camera entity)

### 2. AnimationSystem
```cpp
void AnimationSystem::update(Registry& reg, float dt) {
    for (auto [entity, anim, skeleton] : reg.view<Animation, Skeleton>()) {
        if (!anim.playing) continue;

        anim.time += dt;
        if (anim.time > anim.clip->duration) {
            anim.time = fmod(anim.time, anim.clip->duration);
        }

        // Interpolate keyframes → update skeleton.joints[i].localTransform
        for (auto& channel : anim.clip->channels) {
            interpolateChannel(channel, anim.time, skeleton.joints[channel.jointIndex]);
        }
    }
}
```
- Reads: Animation clips, delta time
- Writes: Joint local transforms

### 3. SkeletonSystem
```cpp
void SkeletonSystem::update(Registry& reg) {
    for (auto [entity, skeleton, transform] : reg.view<Skeleton, Transform>()) {
        std::vector<glm::mat4> globalTransforms(skeleton.joints.size());

        for (size_t i = 0; i < skeleton.joints.size(); i++) {
            auto& joint = skeleton.joints[i];
            if (joint.parentIndex < 0) {
                globalTransforms[i] = joint.localTransform;
            } else {
                globalTransforms[i] = globalTransforms[joint.parentIndex] * joint.localTransform;
            }
            skeleton.boneMatrices[i] = globalTransforms[i] * joint.inverseBindMatrix;
        }
    }
}
```
- Reads: Joint hierarchy, local transforms
- Writes: Bone matrices

### 4. RenderSystem
```cpp
void RenderSystem::update(Registry& reg, Shaders& shaders) {
    // Get active camera
    auto [camEntity, cam, camTransform] = reg.getActiveCamera();
    glm::mat4 view = cam.viewMatrix(camTransform);
    glm::mat4 projection = cam.projectionMatrix(1280.0f / 720.0f);

    // Render all entities with Mesh + Renderable + Transform
    for (auto [entity, mesh, renderable, transform] : reg.view<Mesh, Renderable, Transform>()) {
        Shader& shader = shaders.get(renderable.shader);
        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", transform.matrix());

        if (renderable.shader == ShaderType::Skinned) {
            auto* skeleton = reg.tryGet<Skeleton>(entity);
            if (skeleton) {
                shader.setMat4Array("boneMatrices", skeleton->boneMatrices);
            }
        }

        if (renderable.texture) {
            glBindTexture(GL_TEXTURE_2D, renderable.texture);
        }

        glBindVertexArray(mesh.vao);
        glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
    }
}
```
- Reads: All components
- Writes: GPU state

---

## Registry (Simple Implementation)

```cpp
class Registry {
public:
    Entity create();
    void destroy(Entity e);

    template<typename T>
    T& add(Entity e, T component);

    template<typename T>
    T* tryGet(Entity e);

    template<typename... Ts>
    auto view();  // Returns iterable of (Entity, Ts&...)

private:
    uint32_t nextId = 0;
    std::unordered_map<Entity, Transform> transforms;
    std::unordered_map<Entity, Mesh> meshes;
    std::unordered_map<Entity, Skeleton> skeletons;
    std::unordered_map<Entity, Animation> animations;
    std::unordered_map<Entity, Renderable> renderables;
    std::unordered_map<Entity, Camera> cameras;
};
```

No external ECS library. Simple maps per component type.

---

## Asset Loading (Not a System)

```cpp
// AssetLoader.h
struct LoadedModel {
    std::vector<Mesh> meshes;
    std::optional<Skeleton> skeleton;
    std::vector<AnimationClip> clips;
    std::vector<GLuint> textures;
};

LoadedModel loadGLB(const std::string& path);

// Usage in main.cpp:
auto protagonist = loadGLB("assets/protagonist.glb");
Entity e = registry.create();
registry.add(e, Transform{.scale = glm::vec3(0.01f)});
registry.add(e, protagonist.meshes[0]);
registry.add(e, protagonist.skeleton.value());
registry.add(e, Animation{.clip = &protagonist.clips[0], .playing = true});
registry.add(e, Renderable{.shader = ShaderType::Skinned, .texture = protagonist.textures[0]});
```

---

## New main.cpp

```cpp
int main() {
    // SDL + OpenGL init (same as before)

    // Load shaders
    Shaders shaders;
    shaders.load(ShaderType::Color, "shaders/color.vert", "shaders/color.frag");
    shaders.load(ShaderType::Model, "shaders/model.vert", "shaders/model.frag");
    shaders.load(ShaderType::Skinned, "shaders/skinned.vert", "shaders/model.frag");

    // Create registry
    Registry registry;

    // Load assets and create entities
    auto protagonistData = loadGLB("assets/protagonist.glb");
    Entity protagonist = createCharacter(registry, protagonistData, {0, 0, 0});

    auto snowData = loadGLB("assets/snow_tile.glb");
    for (int x = -2; x <= 2; x++) {
        for (int z = -2; z <= 2; z++) {
            createTile(registry, snowData, {x * 2.0f, 0, z * 2.0f});
        }
    }

    // Create camera
    Entity camera = registry.create();
    registry.add(camera, Transform{.position = {0, 2, 5}});
    registry.add(camera, Camera{.fov = 60.0f, .nearPlane = 0.1f, .farPlane = 100.0f, .active = true});

    // Systems
    InputSystem inputSystem;
    AnimationSystem animationSystem;
    SkeletonSystem skeletonSystem;
    RenderSystem renderSystem;

    // Game loop
    while (running) {
        float dt = computeDeltaTime();

        inputSystem.update(registry, dt);
        animationSystem.update(registry, dt);
        skeletonSystem.update(registry);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderSystem.update(registry, shaders);
        SDL_GL_SwapWindow(window);
    }

    return 0;
}
```

---

## Implementation Order

1. **Create component headers** - Pure data structs
2. **Create Registry** - Simple entity-component storage
3. **Create AssetLoader** - Extract loading logic from Model.cpp
4. **Create RenderSystem** - Extract drawing logic
5. **Create AnimationSystem** - Extract animation update
6. **Create SkeletonSystem** - Extract bone matrix computation
7. **Create InputSystem** - Extract input handling
8. **Rewrite main.cpp** - Wire everything together
9. **Delete old Model.h/cpp, Camera.h** - No longer needed

---

## Key Principles

1. **Components are pure data** - No methods beyond trivial getters
2. **Systems are stateless functions** - Take registry, do one job
3. **No inheritance** - Composition only
4. **No hidden state** - Everything visible in components
5. **Simple over clever** - std::unordered_map, not archetypes

---

## What Stays The Same

- Shader.h/cpp - Already clean, no changes needed
- All shader files - Same uniforms, same logic
- GLB/GLTF loading code - Just reorganized into AssetLoader
- Math operations - GLM usage identical
- SDL initialization - Same window/context setup
