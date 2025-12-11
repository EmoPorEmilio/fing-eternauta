# Fing-Eternauta: Computer Graphics Technical Documentation

## Project Overview

A real-time 3D game engine built from scratch using OpenGL 4.5, featuring a third-person character controller navigating a procedurally generated city. This document details all computer graphics concepts, algorithms, and structures implemented.

---

## 1. Rendering Pipeline

### 1.1 Graphics API
- **OpenGL 4.5 Core Profile**: Modern programmable pipeline with no fixed-function fallbacks
- **GLAD**: OpenGL loader for function pointer resolution
- **SDL3**: Window creation, OpenGL context management, and input handling

### 1.2 Vertex Processing

#### Vertex Buffer Objects (VBO)
All geometry is stored in GPU memory using VBOs with interleaved vertex attributes:
- **Position** (vec3): World-space vertex coordinates
- **Normal** (vec3): Surface normal for lighting calculations
- **Texture Coordinates** (vec2): UV mapping for texture sampling
- **Bone Indices** (ivec4): For skeletal animation (skinned meshes)
- **Bone Weights** (vec4): Blend weights for skeletal animation

#### Vertex Array Objects (VAO)
VAOs encapsulate vertex attribute state, enabling fast geometry switching during rendering.

#### Element Buffer Objects (EBO)
Indexed rendering using EBOs reduces vertex duplication and GPU memory usage.

### 1.3 Shader Programs

#### Vertex Shaders
Transform vertices from model space to clip space through the transformation pipeline:
```
Model Space → World Space → View Space → Clip Space → NDC → Screen Space
```

**Transformation Matrices:**
- **Model Matrix**: Object-to-world transformation (position, rotation, scale)
- **View Matrix**: World-to-camera transformation using `glm::lookAt()`
- **Projection Matrix**: Perspective projection with configurable FOV, near/far planes

#### Fragment Shaders
Compute per-pixel color output with:
- **Texture Sampling**: `texture()` function with sampler2D
- **Lighting Calculations**: Diffuse + ambient illumination
- **Fog Effects**: Exponential squared distance fog
- **Alpha Blending**: For UI elements and transparency

---

## 2. Lighting Model

### 2.1 Phong Illumination (Simplified)

**Ambient Lighting:**
```glsl
float ambient = 0.3;
```
Constant term providing base illumination to prevent completely black shadows.

**Diffuse Lighting (Lambertian Reflectance):**
```glsl
float diff = max(dot(normal, lightDir), 0.0);
```
Light intensity proportional to the angle between surface normal and light direction. Uses the dot product clamped to [0,1].

**Combined Lighting:**
```glsl
float light = ambient + diff * 0.7;
vec3 litColor = textureColor * light;
```

### 2.2 Directional Light
Single infinite-distance light source simulating sunlight:
```cpp
glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
```

---

## 3. Texture Mapping

### 3.1 2D Texture Mapping
- **UV Coordinates**: Per-vertex texture coordinates mapping geometry to texture space
- **Texture Wrapping**: `GL_REPEAT` for tiled ground textures
- **Texture Filtering**:
  - `GL_LINEAR_MIPMAP_LINEAR` for minification (trilinear filtering)
  - `GL_LINEAR` for magnification (bilinear filtering)

### 3.2 Mipmapping
Automatic mipmap generation with `glGenerateMipmap()` for:
- Reduced aliasing at distance
- Improved texture cache performance
- Anisotropic filtering compatibility

### 3.3 Tiled/Repeating Textures
Ground plane uses scaled UV coordinates for seamless tiling:
```cpp
float uvScale = planeSize * texScale;  // World units to UV scale
```

---

## 4. Skeletal Animation

### 4.1 Bone Hierarchy
- **Joint Tree**: Hierarchical skeleton structure loaded from glTF/GLB files
- **Bind Pose**: Reference pose where skin is attached to skeleton
- **Inverse Bind Matrices**: Transform from mesh space to bone space

### 4.2 Keyframe Interpolation
Animation data stored as keyframes with:
- **Translation Keys**: Position over time
- **Rotation Keys**: Quaternion orientation over time
- **Scale Keys**: Uniform/non-uniform scaling over time

**Linear Interpolation (LERP)** for position/scale:
```cpp
glm::mix(key1.value, key2.value, t);
```

**Spherical Linear Interpolation (SLERP)** for rotations:
```cpp
glm::slerp(quat1, quat2, t);
```

### 4.3 Skinning (Linear Blend Skinning / LBS)
Each vertex influenced by up to 4 bones:
```glsl
mat4 skinMatrix = uBones[boneIndices.x] * boneWeights.x
                + uBones[boneIndices.y] * boneWeights.y
                + uBones[boneIndices.z] * boneWeights.z
                + uBones[boneIndices.w] * boneWeights.w;
vec4 skinnedPos = skinMatrix * vec4(aPosition, 1.0);
```

### 4.4 Bone Matrix Computation
```cpp
boneMatrix = globalTransform * inverseBindMatrix;
```
Transforms vertices from bind pose to animated pose.

---

## 5. Camera System

### 5.1 Third-Person Camera
Over-the-shoulder view with configurable parameters:
- **Distance**: Camera distance behind target
- **Height**: Vertical offset above target
- **Shoulder Offset**: Horizontal offset for asymmetric framing
- **Look Ahead**: Point of interest ahead of character

### 5.2 View Matrix Construction
```cpp
glm::mat4 view = glm::lookAt(cameraPosition, lookAtTarget, upVector);
```

### 5.3 Perspective Projection
```cpp
glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
```
- **Field of View**: 60 degrees
- **Near Plane**: 0.1 units (prevents z-fighting)
- **Far Plane**: 100 units (rendering distance limit)

### 5.4 Camera Orbit Controls
- **Yaw**: Horizontal rotation (mouse X movement)
- **Pitch**: Vertical rotation (mouse Y movement) clamped to [-60, +60] degrees

---

## 6. Procedural Geometry Generation

### 6.1 Box Mesh Generation
Procedural unit cube (1x1x1) with:
- **24 vertices**: 4 per face for correct flat shading normals
- **36 indices**: 12 triangles (2 per face)
- **Face Normals**: Each face has uniform normal direction

```cpp
// 6 faces × 4 vertices × 8 floats (pos + normal + uv)
std::vector<float> vertices = { ... };
```

### 6.2 Procedural City Grid
100×100 building grid (10,000 buildings) with:
- **Random Heights**: Uniform distribution [15, 40] units
- **Street Gaps**: 12-unit spacing between buildings
- **Transform Scaling**: Single unit-cube mesh scaled per-building

---

## 7. Spatial Data Structures & Culling

### 7.1 Grid-Based Spatial Culling
Efficient O(1) lookup for nearby entities using grid coordinates:
```cpp
int gridX = floor((position.x - offset) / blockSize);
int gridZ = floor((position.z - offset) / blockSize);
```

### 7.2 Distance-Based Culling
Only buildings within a configurable grid radius are rendered:
- **Render Radius**: 3 cells (7×7 = 49 buildings visible)
- **Chebyshev Distance**: `max(|dx|, |dz|) <= radius`

### 7.3 Entity Pool Pattern
Fixed pool of rendering entities reused for visible buildings:
- Avoids creating/destroying entities at runtime
- Buildings outside range are hidden (Y = -1000)
- Pool size matches maximum visible count

### 7.4 AABB (Axis-Aligned Bounding Box)
Bounding volume for spatial queries:
```cpp
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};
```

---

## 8. Atmospheric Effects

### 8.1 Exponential Squared Fog
Distance-based fog using exponential squared falloff:
```glsl
float distance = length(fragPos - viewPos);
float fogFactor = 1.0 - exp(-pow(fogDensity * distance, 2.0));
fogFactor = clamp(fogFactor, 0.0, 1.0);
```

### 8.2 Fog Blending
Color interpolation with desaturation effect:
```glsl
float gray = luminance(litColor);  // Grayscale conversion
vec3 desaturated = mix(litColor, vec3(gray), fogFactor * fogDesaturation);
vec3 finalColor = mix(desaturated, fogColor, fogFactor);
```

### 8.3 Fog Parameters
- **Fog Color**: Matches background (0.1, 0.1, 0.12)
- **Fog Density**: 0.02 (configurable via pause menu)
- **Desaturation Factor**: 0.8 (reduces saturation with distance)

---

## 9. Level of Detail (LOD)

### 9.1 Distance-Based LOD Switching
Mesh quality selection based on camera distance:
```cpp
float distance = glm::length(cameraPos - objectPos);
bool useHighDetail = distance < lodSwitchDistance;  // 70 units
```

### 9.2 LOD Mesh Swapping
Hot-swapping mesh references without entity recreation:
```cpp
meshGroup->meshes = useHighDetail ? highDetailMeshes : lowDetailMeshes;
```

---

## 10. User Interface Rendering

### 10.1 Orthographic Projection
2D UI overlay using screen-space coordinates:
```cpp
glm::mat4 uiProjection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
```

### 10.2 Text Rendering
- **SDL_ttf**: TrueType font rasterization
- **Texture Caching**: Pre-rendered text textures for performance
- **Anchor Points**: Flexible positioning (Center, BottomCenter, etc.)

### 10.3 Minimap Rendering

#### Circular Minimap
Fragment shader discards pixels outside circle:
```glsl
if (length(fragPos - center) > radius) discard;
```

#### Building Footprints
Rotated rectangles rendered with per-building transform:
```glsl
vec2 rotated;
rotated.x = scaled.x * cos(rotation) - scaled.y * sin(rotation);
rotated.y = scaled.x * sin(rotation) + scaled.y * cos(rotation);
```

#### Cardinal Direction Indicators
Text markers (N, E, S, W) rotating with player orientation.

### 10.4 Alpha Blending
UI transparency using standard blend function:
```cpp
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

---

## 11. Collision Detection

### 11.1 AABB vs Point
Simple containment test for ground collision:
```cpp
bool grounded = (position.y <= groundLevel);
```

### 11.2 Box Colliders
Axis-aligned box collision volumes with half-extents and offset.

---

## 12. Physics Simulation

### 12.1 Euler Integration
Simple velocity integration:
```cpp
velocity.y += gravity * dt;
position += velocity * dt;
```

### 12.2 Ground Clamping
Position correction on ground contact:
```cpp
if (position.y < groundLevel) {
    position.y = groundLevel;
    velocity.y = 0;
    grounded = true;
}
```

---

## 13. Entity Component System (ECS)

### 13.1 Architecture
- **Entities**: Unique integer identifiers
- **Components**: Plain data structures (Transform, Mesh, Renderable, etc.)
- **Systems**: Stateless processors operating on component groups

### 13.2 Component Types
| Component | Data |
|-----------|------|
| Transform | position, rotation (quaternion), scale |
| MeshGroup | VAO, index count, texture handles |
| Renderable | shader type (Color/Model/Skinned/Terrain), mesh offset |
| Camera | FOV, near/far planes, active flag |
| Skeleton | bone matrices, joint hierarchy |
| Animation | clip index, time, playing state, clips vector |
| RigidBody | velocity, gravity, grounded flag |
| PlayerController | move speed, turn speed |
| FacingDirection | yaw angle, turn speed |
| FollowTarget | camera-player relationship |
| BoxCollider | half extents, offset |
| UIText | text, fontId, anchor, offset, color, visible |
| AABB | min/max bounds for collision/culling |

### 13.3 Render System
Iterates all entities with Transform + MeshGroup + Renderable, setting uniforms and issuing draw calls.

---

## 14. Model Loading (glTF/GLB)

### 14.1 glTF Format
Industry-standard 3D format supporting:
- Mesh geometry with indexed primitives
- PBR materials and textures
- Skeletal animation with bone hierarchy
- Binary blob storage (GLB)

### 14.2 Data Extraction
- **Accessors**: Typed views into binary buffers
- **Buffer Views**: Byte ranges within buffers
- **Attribute Mapping**: Position, normal, UV, bone data

---

## 15. Depth Testing

### 15.1 Z-Buffer Algorithm
Hardware depth testing for correct occlusion:
```cpp
glEnable(GL_DEPTH_TEST);
```

### 15.2 Depth Buffer Clearing
Per-frame depth buffer reset:
```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

---

## 16. Shader Uniform Types

| Uniform | Type | Purpose |
|---------|------|---------|
| uModel | mat4 | Model transformation matrix |
| uView | mat4 | View transformation matrix |
| uProjection | mat4 | Projection matrix |
| uBones[] | mat4[] | Bone transformation array |
| uLightDir | vec3 | Directional light direction |
| uViewPos | vec3 | Camera position (for fog) |
| uTexture | sampler2D | Diffuse texture |
| uHasTexture | int | Texture presence flag |
| uFogEnabled | int | Fog toggle |
| uUseSkinning | int | Skeletal animation toggle |

---

## 17. Coordinate Systems

### 17.1 Spaces Used
- **Model Space**: Object-local coordinates
- **World Space**: Global scene coordinates (Y-up, right-handed)
- **View Space**: Camera-relative coordinates
- **Clip Space**: Homogeneous coordinates after projection
- **NDC**: Normalized Device Coordinates [-1, 1]
- **Screen Space**: Pixel coordinates

### 17.2 Quaternion Rotations
All rotations stored as quaternions to avoid gimbal lock:
```cpp
glm::quat rotation = glm::angleAxis(radians, axis);
transform.rotation = glm::slerp(current, target, t);
```

---

## 18. Double Buffering

Swap chain with back buffer rendering:
```cpp
SDL_GL_SwapWindow(window);
```
Prevents screen tearing and provides smooth frame presentation.

---

## 19. File Structure

```
src/
├── ecs/
│   ├── components/     # Data-only structs
│   ├── systems/        # Processing logic
│   └── Registry.h      # Entity-component storage
├── assets/
│   └── AssetLoader.cpp # glTF/GLB parsing
├── core/
│   ├── GameConfig.h    # Compile-time constants
│   ├── GameState.h     # Runtime state variables
│   └── WindowManager.h # SDL/OpenGL context
├── culling/
│   ├── BuildingCuller.h    # Octree + frustum culling
│   ├── Frustum.h           # Frustum plane extraction
│   └── Octree.h            # Spatial partitioning
├── procedural/
│   └── BuildingGenerator.h  # Procedural mesh generation
├── rendering/
│   ├── SceneRenderer.h     # 3D scene rendering abstraction
│   ├── InstancedRenderer.h # GPU instancing utilities
│   └── RenderPipeline.h    # Render pipeline management
├── scenes/
│   ├── IScene.h            # Scene interface
│   ├── SceneContext.h      # Shared resource container
│   ├── SceneManager.h      # Scene lifecycle
│   ├── RenderHelpers.h     # Shared rendering utilities
│   └── impl/               # Scene implementations
├── ui/
│   ├── FontManager.h   # Font loading
│   └── TextCache.h     # Text texture caching
└── Shader.h            # Shader compilation/linking

shaders/
├── model.vert/frag         # Standard lit shader
├── skinned.vert            # Skeletal animation shader
├── color.vert/frag         # Debug/solid color shader
├── ui.vert/frag            # 2D UI rendering
├── minimap.vert/frag       # Circular minimap
├── minimap_rect.vert/frag  # Building footprints
├── terrain.vert/frag       # Ground plane shader
├── building_instanced.vert # Instanced building rendering
├── depth.vert/frag         # Shadow pass depth shader
├── depth_instanced.vert    # Instanced shadow pass
├── comet.vert/frag         # Animated comet effect
├── sun.vert/frag           # Sun billboard
├── motion_blur.vert/frag   # Velocity-based motion blur
├── toon_post.vert/frag     # Comic/toon post-process
├── shadertoy_overlay.vert/frag  # Procedural snow effect
├── solid_overlay.vert/frag # Solid color overlay
├── fullscreen.vert         # Fullscreen quad vertex shader
└── blit.frag               # Simple texture blit
```

---

## 20. Performance Optimizations

1. **Shared Mesh References**: 10,000 buildings share one VAO
2. **Entity Pooling**: Fixed allocation, no runtime allocs
3. **Grid-Based Culling**: O(1) visibility determination
4. **Dirty Flag Updates**: Culling only recomputes on cell change
5. **Texture Caching**: Pre-rendered UI text
6. **Mipmapping**: Reduced texture bandwidth at distance
7. **Early Fragment Discard**: Minimap circle clipping in shader
8. **Spatial Culling**: Octree + frustum culling for O(log n) visibility queries
9. **Instanced Rendering**: Single draw call for all visible buildings

---

## 21. Spatial Culling Architecture

### 21.1 Octree Structure
Buildings are stored in an octree for efficient spatial queries:

```cpp
template<typename T>
class Octree {
    static constexpr int MAX_OBJECTS_PER_NODE = 8;
    static constexpr int MAX_DEPTH = 8;

    struct Node {
        AABB bounds;
        std::vector<const T*> objects;
        std::array<std::unique_ptr<Node>, 8> children;
        bool isLeaf = true;
    };

    void queryFrustum(const Frustum& frustum, std::function<void(const T&)> callback);
    void queryRadius(const glm::vec3& center, float radius, std::function<void(const T&)> callback);
};
```

**Octree Benefits:**
- O(log n) visibility queries vs O(n) linear scan
- Early rejection of entire octree branches outside frustum
- Supports both frustum culling and radius queries (for collision)

### 21.2 Frustum Extraction (Gribb-Hartmann Method)
Frustum planes extracted from view-projection matrix:

```cpp
void Frustum::extractFromMatrix(const glm::mat4& viewProjection) {
    // Left plane: row 3 + row 0
    m_planes[LEFT].normal.x = viewProjection[0][3] + viewProjection[0][0];
    // ... 6 planes total (left, right, top, bottom, near, far)

    // Normalize all planes
    for (auto& plane : m_planes) {
        plane.normalize();
    }
}
```

### 21.3 AABB-Frustum Intersection
Uses p-vertex test for early rejection:

```cpp
bool Frustum::isBoxOutside(const AABB& box) const {
    for (const auto& plane : m_planes) {
        // Find corner most aligned with plane normal (p-vertex)
        glm::vec3 pVertex;
        pVertex.x = (plane.normal.x >= 0) ? box.max.x : box.min.x;
        pVertex.y = (plane.normal.y >= 0) ? box.max.y : box.min.y;
        pVertex.z = (plane.normal.z >= 0) ? box.max.z : box.min.z;

        // If p-vertex behind plane, box is completely outside
        if (plane.distanceToPoint(pVertex) < 0) return true;
    }
    return false;
}
```

### 21.4 BuildingCuller Orchestration
Combines octree queries with instanced rendering:

```cpp
class BuildingCuller {
    void update(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, float maxRenderDistance) {
        m_frustum.extractFromMatrix(projection * view);
        m_instancedRenderer.beginFrame();

        m_octree.queryFrustum(m_frustum, [&](const BuildingData& building) {
            float distSq = glm::dot(building.position - cameraPos, ...);
            if (distSq <= maxRenderDistance * maxRenderDistance) {
                m_instancedRenderer.addInstance(building.position, building.scale);
            }
        });
    }
};
```

### 21.5 Uniform Location Caching
The `Shader` class caches uniform locations to avoid repeated `glGetUniformLocation` calls:

```cpp
GLint Shader::getUniformLocation(const char* name) const
{
    // Check cache first
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end()) {
        return it->second;
    }

    // Query OpenGL and cache the result
    GLint location = glGetUniformLocation(m_program, name);
    m_uniformCache[name] = location;
    return location;
}
```

**Performance Impact:**
- First access: O(1) hash lookup + OpenGL query
- Subsequent accesses: O(1) hash lookup only
- Eliminates string comparison overhead in driver

### 21.6 Instanced Rendering (Buildings)
Buildings can be rendered using GPU instancing for massive draw call reduction:

```cpp
// InstancedRenderer stores transforms in GPU buffer
class InstancedRenderer {
    void addInstance(const glm::vec3& position, const glm::vec3& scale);
    void render(const Mesh& mesh, Shader& shader);
};

// Instanced vertex shader receives model matrix per-instance
layout (location = 5) in mat4 aInstanceModel;  // 4 vec4 slots
```

**Implementation Details:**
- Model matrices stored in instance buffer (GL_DYNAMIC_STORAGE_BIT)
- Matrix passed via vertex attributes with divisor=1
- Single `glDrawElementsInstanced` call for all visible buildings

**Performance Impact:**
- **Before**: 49 draw calls (7×7 visible buildings)
- **After**: 1 draw call with 49 instances
- ~50x reduction in CPU-GPU command overhead

### 21.7 Ray-AABB Intersection (Slab Method)
Used for camera collision detection against buildings:

```cpp
bool AABB::raycast(const glm::vec3& origin, const glm::vec3& dirInv,
                   float maxDist, float& tMin) const {
    // Compute intersection distances for each axis slab
    float t1 = (min.x - origin.x) * dirInv.x;
    float t2 = (max.x - origin.x) * dirInv.x;
    float t3 = (min.y - origin.y) * dirInv.y;
    float t4 = (max.y - origin.y) * dirInv.y;
    float t5 = (min.z - origin.z) * dirInv.z;
    float t6 = (max.z - origin.z) * dirInv.z;

    // tmin = entry point, tmax = exit point
    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    // Ray misses or box is behind
    if (tmax < 0 || tmin > tmax) return false;

    tMin = (tmin < 0) ? tmax : tmin;  // Handle origin inside box
    return tMin <= maxDist;
}
```

**Key Concepts:**
- `dirInv` = 1.0 / direction (precomputed for efficiency)
- Slab method tests ray against 3 pairs of parallel planes
- Entry point is maximum of all minimum slab intersections
- Exit point is minimum of all maximum slab intersections

### 21.8 Octree Raycast Traversal
The octree supports raycasting for camera collision:

```cpp
void raycastNode(const Node* node, const glm::vec3& origin,
                 const glm::vec3& dirInv, float& closestHit) const {
    // Check if ray intersects node bounds OR origin is inside node
    float nodeDist;
    bool hitsNode = node->bounds.raycast(origin, dirInv, 1e10f, nodeDist);

    if (!hitsNode && !node->bounds.contains(origin)) {
        return;  // Early rejection
    }

    // Test all objects in this node
    for (const T* obj : node->objects) {
        AABB objBounds = m_getAABB(*obj);
        float objDist;
        if (objBounds.raycast(origin, dirInv, closestHit, objDist)) {
            if (objDist < closestHit && objDist >= 0.0f) {
                closestHit = objDist;
            }
        }
    }

    // Recurse into children
    if (!node->isLeaf) {
        for (const auto& child : node->children) {
            if (child) raycastNode(child.get(), origin, dirInv, closestHit);
        }
    }
}
```

**Important:** The `contains(origin)` check handles the case where the ray starts inside a node (common when camera is inside the world bounds).

---

## 22. Camera Collision System

### 22.1 Overview
Prevents the camera from clipping through buildings by casting a ray from the character toward the desired camera position.

### 22.2 Ray Origin Selection
The ray is cast from the **character's shoulder position**, not the look-at point:

```cpp
glm::vec3 characterPos = targetTransform->position;
characterPos.y += 1.5f;  // Shoulder height

camTransform.position = resolveCollision(characterPos, desiredCamPos, culler, extraAABB);
```

**Why shoulder position?**
- The look-at point is ahead of the character (where camera looks)
- Using look-at as ray origin would detect buildings in front of the character
- We only want to detect walls between character and camera (behind)

### 22.3 Collision Resolution
When a hit is detected, camera is moved to the wall surface minus an offset:

```cpp
glm::vec3 resolveCollision(const glm::vec3& origin, const glm::vec3& desiredCamPos,
                            const BuildingCuller& culler, const AABB* extraAABB) {
    glm::vec3 toCamera = desiredCamPos - origin;
    float desiredDist = glm::length(toCamera);
    glm::vec3 direction = toCamera / desiredDist;

    float hitDist;
    if (culler.raycastWithExtra(origin, direction, desiredDist, extraAABB, hitDist)) {
        // Pull camera in front of wall
        float newDist = glm::max(hitDist - COLLISION_OFFSET, 0.1f);
        return origin + direction * newDist;
    }

    return desiredCamPos;  // No obstruction
}
```

### 22.4 Extra AABB Support
Non-octree objects (like the FING building) can be included via `extraAABB` parameter:

```cpp
bool raycastWithExtra(const glm::vec3& origin, const glm::vec3& direction,
                      float maxDist, const AABB* extraAABB, float& hitDist) const {
    float octreeHit = maxDist;
    bool hitOctree = m_octree.raycast(origin, direction, maxDist, octreeHit);

    float extraHit = maxDist;
    bool hitExtra = extraAABB && extraAABB->raycast(origin, dirInv, maxDist, extraHit);

    // Return closest hit
    if (hitOctree && hitExtra) {
        hitDist = glm::min(octreeHit, extraHit);
        return true;
    }
    // ... handle single hits
}
```

---

## 23. Character-Building Collision

### 23.1 Octree-Based Collision Detection
Character collision uses octree radius queries instead of iterating all entities:

```cpp
void resolveCollisions(const BuildingCuller* buildingCuller, const AABB* extraAABB) {
    if (buildingCuller) {
        buildingCuller->queryRadius(newPos, COLLISION_QUERY_RADIUS,
            [&](const BuildingGenerator::BuildingData& building) {
                // Build AABB for this building
                glm::vec3 halfExtents(building.width * 0.5f, building.height * 0.5f, building.depth * 0.5f);
                glm::vec3 center = building.position + glm::vec3(0.0f, building.height * 0.5f, 0.0f);
                AABB buildingAABB = AABB::fromCenterExtents(center, halfExtents);
                checkAABB(buildingAABB);
            });
    }
}
```

### 23.2 AABB Expansion for Player Radius
Buildings are expanded by player radius for circle-vs-AABB collision:

```cpp
auto checkAABB = [&](const AABB& aabb) {
    AABB expanded = aabb;
    expanded.min.x -= PLAYER_RADIUS;
    expanded.min.z -= PLAYER_RADIUS;
    expanded.max.x += PLAYER_RADIUS;
    expanded.max.z += PLAYER_RADIUS;

    // Check if player point is inside expanded box
    if (newPos.x > expanded.min.x && newPos.x < expanded.max.x &&
        newPos.z > expanded.min.z && newPos.z < expanded.max.z) {
        // Resolve collision...
    }
};
```

### 23.3 Sliding Collision Response
When collision is detected, player slides along walls using minimum penetration axis:

```cpp
// Calculate penetration on each axis
float penLeft = newPos.x - expanded.min.x;
float penRight = expanded.max.x - newPos.x;
float penBack = newPos.z - expanded.min.z;
float penFront = expanded.max.z - newPos.z;

float minPenX = std::min(penLeft, penRight);
float minPenZ = std::min(penBack, penFront);

// Push out on axis with least penetration
if (minPenX < minPenZ) {
    newPos.x = (penLeft < penRight) ? expanded.min.x : expanded.max.x;
} else {
    newPos.z = (penBack < penFront) ? expanded.min.z : expanded.max.z;
}
```

This creates smooth sliding behavior along walls instead of stopping abruptly.

---

## 24. Libraries Used

| Library | Purpose |
|---------|---------|
| OpenGL 4.5 | Graphics API |
| GLAD | OpenGL loader |
| SDL3 | Windowing, input, context |
| SDL3_ttf | Font rendering |
| GLM | Math library (vectors, matrices, quaternions) |
| stb_image | Image loading |
| cgltf | glTF/GLB parsing |

---

## 25. Scene System Architecture

### 25.1 Overview
The scene system provides a clean separation between game states (menu, gameplay, cinematics) using an OOP-based scene management approach that works alongside the ECS architecture.

**Design Philosophy: Hybrid ECS + OOP**
- **ECS** handles entities, components, and low-level systems (rendering, physics, animation)
- **OOP Scenes** handle high-level game flow, state transitions, and scene-specific logic
- This is a pragmatic approach used by many production engines (Unity, Unreal)

### 25.2 IScene Interface
Base interface for all game scenes with lifecycle methods:

```cpp
class IScene {
public:
    virtual ~IScene() = default;
    virtual void onEnter(SceneContext& ctx) = 0;   // Called when scene becomes active
    virtual void update(SceneContext& ctx) = 0;    // Called every frame
    virtual void render(SceneContext& ctx) = 0;    // Called every frame after update
    virtual void onExit(SceneContext& ctx) = 0;    // Called when leaving scene
};
```

**Lifecycle Flow:**
```
Scene A (active) → switchTo(B) → A.onExit() → B.onEnter() → B.update/render loop
```

### 25.3 SceneContext (Dependency Injection)
A struct containing all shared resources passed to scenes. Scenes don't own these resources - they're owned by `main.cpp`:

```cpp
struct SceneContext {
    // Core ECS
    Registry* registry;
    GameState* gameState;
    SceneManager* sceneManager;

    // Input (updated each frame)
    InputSystem* inputSystem;
    InputState input;
    float dt;
    float aspectRatio;

    // Systems
    RenderSystem* renderSystem;
    UISystem* uiSystem;
    MinimapSystem* minimapSystem;
    CinematicSystem* cinematicSystem;
    AnimationSystem* animationSystem;
    // ... more systems

    // Shaders, FBOs, textures, VAOs
    Shader* groundShader;
    GLuint msaaFBO;
    GLuint snowTexture;
    // ... more GPU resources

    // Key entities
    Entity protagonist;
    Entity camera;
    Entity fingBuilding;
};
```

**Benefits:**
- Scenes remain stateless and testable
- No global state or singletons
- Clear ownership model (main.cpp owns everything)
- Easy to mock for testing

### 25.4 SceneManager
Handles scene registration, transitions, and lifecycle orchestration:

```cpp
class SceneManager {
public:
    void registerScene(SceneType type, std::unique_ptr<IScene> scene);
    void switchTo(SceneType type);           // Request transition (deferred)
    void processTransitions(SceneContext& ctx);  // Execute pending transitions
    void update(SceneContext& ctx);
    void render(SceneContext& ctx);

    SceneType current() const;
    SceneType previous() const;              // For pause menu resume

private:
    std::unordered_map<SceneType, std::unique_ptr<IScene>> m_scenes;
    SceneType m_current = SceneType::MainMenu;
    SceneType m_previous = SceneType::MainMenu;
    SceneType m_pending = SceneType::MainMenu;
    bool m_hasPending = false;
};
```

**Deferred Transitions:**
Scene switches are deferred to prevent issues when switching mid-update:
```cpp
void switchTo(SceneType type) {
    m_pending = type;
    m_hasPending = true;  // Processed next frame
}
```

### 25.5 Scene Types
```cpp
enum class SceneType {
    MainMenu,        // Static backdrop, menu navigation
    IntroText,       // Typewriter text reveal
    IntroCinematic,  // Camera path with motion blur
    PlayGame,        // Full gameplay with all systems
    GodMode,         // Free camera debug mode
    PauseMenu        // Settings overlay
};
```

### 25.6 Scene Implementations

#### MainMenuScene
- Static camera backdrop showing FING building
- Falling comets in background
- 85% black overlay for readability
- Up/Down navigation, Enter to select

#### IntroTextScene
- Typewriter effect using per-character reveal
- Multiple lines with configurable delays
- Skip with Enter/Escape

#### IntroCinematicScene
- NURBS camera path animation
- GPU Gems velocity-based motion blur
- Shadow pass + full scene rendering
- Transitions to PlayGame on completion

#### PlayGameScene
- Full gameplay: player movement, camera follow
- Shadow mapping with PCF
- Building collision (player and camera)
- Minimap with markers
- Toon post-processing (optional)

#### GodModeScene
- Free camera (WASD + mouse)
- No shadows for performance
- Debug camera position logging (P key)
- Useful for level design and debugging

#### PauseMenuScene
- Black background with UI overlay
- Settings toggles: Fog, Snow, Toon mode
- Slider adjustments: Snow speed, angle, blur
- Returns to previous scene (PlayGame or GodMode)

### 25.7 Rendering Pipeline in Scenes

Each scene manages its own rendering pipeline:

```cpp
void PlayGameScene::render(SceneContext& ctx) {
    // 1. Shadow pass
    renderShadowPass(ctx, lightSpaceMatrix, cameraPos);

    // 2. Main render to MSAA FBO (or toon FBO if enabled)
    GLuint targetFBO = ctx.gameState->toonShadingEnabled ? ctx.toonFBO : ctx.msaaFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);

    // 3. Render scene content
    ctx.renderSystem->update(*ctx.registry, ctx.aspectRatio);
    renderBuildings(ctx, view, projection, lightSpaceMatrix, cameraPos);
    RenderHelpers::renderGroundPlane(...);
    renderSun(...);
    renderComets(...);
    RenderHelpers::renderSnowOverlay(...);

    // 4. Toon post-processing (if enabled)
    if (ctx.gameState->toonShadingEnabled) {
        renderToonPostProcess(ctx);
    }

    // 5. Minimap and UI
    ctx.minimapSystem->render(...);
    ctx.uiSystem->update(...);
}
```

### 25.8 RenderHelpers (Shared Rendering Code)
Static utility functions for common rendering tasks shared across scenes:

```cpp
namespace RenderHelpers {
    // Compute light-space matrix for shadow mapping
    glm::mat4 computeLightSpaceMatrix(const glm::vec3& focusPoint, const glm::vec3& lightDir);

    // Render textured ground plane with optional shadows
    void renderGroundPlane(Shader& shader, const glm::mat4& view, const glm::mat4& projection,
                           const glm::mat4& lightSpaceMatrix, const glm::vec3& lightDir,
                           const glm::vec3& viewPos, bool fogEnabled, bool shadowsEnabled,
                           GLuint texture, GLuint shadowMap, GLuint vao);

    // Render fullscreen snow overlay
    void renderSnowOverlay(Shader& shader, GLuint vao, const GameState& state);

    // Configure render system for shadow/lighting
    void setupRenderSystem(RenderSystem& rs, bool fog, bool shadows,
                           GLuint shadowTex, const glm::mat4& lightSpace);

    // LOD switching for FING building
    void updateFingLOD(Registry& reg, GameState& state, Entity fingBuilding,
                       const glm::vec3& viewerPos, MeshGroup& highDetail,
                       MeshGroup& lowDetail, float switchDistance);
}
```

### 25.9 MSAA Resolve (Final Pass)
The MSAA resolve happens in `main.cpp` after scene rendering, shared by all 3D scenes:

```cpp
// In game loop, after sceneManager.render(sceneCtx)
if (currentScene == SceneType::IntroCinematic ||
    currentScene == SceneType::PlayGame ||
    currentScene == SceneType::GodMode) {

    // Step 1: Resolve MSAA FBO to regular texture FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Step 2: Blit resolved texture to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    blitShader.use();
    glBindTexture(GL_TEXTURE_2D, resolveColorTex);
    // Draw fullscreen quad
}
```

### 25.10 File Structure After Refactoring

```
src/
├── scenes/
│   ├── IScene.h              # Base interface
│   ├── SceneContext.h        # Shared resource container
│   ├── SceneManager.h        # Scene lifecycle management
│   ├── RenderHelpers.h       # Shared rendering utilities
│   └── impl/
│       ├── MainMenuScene.h
│       ├── IntroTextScene.h
│       ├── IntroCinematicScene.h
│       ├── PlayGameScene.h
│       ├── GodModeScene.h
│       └── PauseMenuScene.h
├── ecs/
│   ├── components/           # Data-only structs
│   ├── systems/              # ECS processing logic
│   └── Registry.h            # Entity-component storage
├── core/
│   ├── GameConfig.h          # Compile-time constants
│   ├── GameState.h           # Runtime state variables
│   └── WindowManager.h       # SDL/OpenGL context
└── ...

main.cpp                      # ~1170 lines (down from ~1990)
  - Initialization
  - Asset loading
  - Entity creation
  - SceneContext population
  - Game loop shell
  - MSAA resolve
  - Cleanup
```

---

## 26. Shadow Mapping

### 26.1 Overview
Directional shadow mapping using a dedicated depth-only render pass.

### 26.2 Shadow Pass
```cpp
// Configure shadow FBO
glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);  // 2048x2048
glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
glClear(GL_DEPTH_BUFFER_BIT);

// Compute light-space matrix centered on player
glm::vec3 lightPos = playerPos + lightDir * SHADOW_DISTANCE;
glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize,
                                        SHADOW_NEAR, SHADOW_FAR);
glm::mat4 lightView = glm::lookAt(lightPos, playerPos, glm::vec3(0, 1, 0));
glm::mat4 lightSpaceMatrix = lightProjection * lightView;

// Render shadow casters
depthShader.use();
depthShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
// ... render buildings and FING
```

### 26.3 Shadow Sampling (Main Pass)
```glsl
// In fragment shader
float shadow = 0.0;
vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
projCoords = projCoords * 0.5 + 0.5;  // [-1,1] -> [0,1]

float closestDepth = texture(uShadowMap, projCoords.xy).r;
float currentDepth = projCoords.z;
float bias = 0.005;  // Prevents shadow acne

shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
```

---

## 27. Motion Blur (Cinematic)

### 27.1 GPU Gems Velocity-Based Motion Blur
Used during intro cinematic for smooth camera movement.

### 27.2 Pipeline
1. Render scene to MSAA FBO
2. Resolve MSAA to motion blur FBO (color + depth)
3. Apply motion blur post-process
4. Output to final MSAA FBO for resolve

### 27.3 Velocity Reconstruction
```glsl
// Reconstruct world position from depth
vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
vec4 worldPos = uInvViewProjection * clipPos;
worldPos /= worldPos.w;

// Compute previous frame position
vec4 prevClip = uPrevViewProjection * worldPos;
prevClip /= prevClip.w;

// Velocity in screen space
vec2 velocity = (currentUV - prevUV) * uBlurStrength;
```

### 27.4 Blur Accumulation
```glsl
vec3 color = vec3(0.0);
for (int i = 0; i < uNumSamples; i++) {
    vec2 offset = velocity * (float(i) / float(uNumSamples - 1) - 0.5);
    color += texture(uColorBuffer, uv + offset).rgb;
}
color /= float(uNumSamples);
```

---

## 28. Toon/Comic Post-Processing

### 28.1 Overview
Optional stylized rendering with edge detection and color quantization.

### 28.2 Pipeline
1. Render scene to toonFBO (non-MSAA texture)
2. Apply toon post-process shader
3. Output to MSAA FBO for final resolve

### 28.3 Edge Detection (Sobel)
```glsl
// Sample neighbors for edge detection
float samples[9];
// ... gather 3x3 neighborhood luminance

// Sobel kernels
float gx = samples[0] - samples[2] + 2.0*(samples[3] - samples[5]) + samples[6] - samples[8];
float gy = samples[0] - samples[6] + 2.0*(samples[1] - samples[7]) + samples[2] - samples[8];
float edge = sqrt(gx*gx + gy*gy);

// Draw black edge lines
if (edge > edgeThreshold) {
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
```

---

## 29. Comet Sky Effect

### 29.1 Instanced Rendering
12 comets rendered with a single instanced draw call:

```cpp
// Instance data: position.xyz + timeOffset.w
std::vector<glm::vec4> cometInstances(NUM_COMETS);
for (int i = 0; i < NUM_COMETS; i++) {
    cometInstances[i] = glm::vec4(pos.x, pos.y, pos.z, randomTimeOffset);
}

// Setup instance VBO
glBindVertexArray(cometMesh.vao);
glBindBuffer(GL_ARRAY_BUFFER, cometInstanceVBO);
glEnableVertexAttribArray(3);
glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), 0);
glVertexAttribDivisor(3, 1);  // Per-instance
```

### 29.2 Animation in Shader
```glsl
// Compute falling position based on time
float t = mod(uTime + aInstanceData.w, uCycleTime) / uCycleTime;
vec3 offset = uFallDirection * t * uFallDistance;
vec3 worldPos = aInstanceData.xyz + offset * uFallSpeed;

// Stretch mesh along fall direction for trail effect
vec3 stretchedPos = aPosition;
stretchedPos += uFallDirection * aPosition.y * uTrailStretch;
```

---

## 30. Snow Overlay Effect

### 30.1 Shadertoy-Style Fullscreen Effect
Procedural snow particles rendered as a post-process overlay:

```glsl
// Layered noise for depth
for (int layer = 0; layer < 3; layer++) {
    float scale = 1.0 + float(layer) * 0.5;
    vec2 p = uv * scale + vec2(uTime * uSpeed, uTime * 0.2);

    // Simplex noise for particle positions
    float n = snoise(p * 100.0);

    // Fade particles based on wind angle
    float angle = atan(uWindDir.y, uWindDir.x);
    float fade = 1.0 - abs(dot(normalize(p), uWindDir));

    snow += n * fade * layerAlpha[layer];
}
```

### 30.2 Configurable Parameters
- **Speed**: Particle fall rate (0.1 - 10.0)
- **Angle**: Wind direction (-180° to 180°)
- **Motion Blur**: Trail length (0.0 - 5.0)
- **Enabled**: Toggle via pause menu
