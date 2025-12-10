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
| Renderable | shader type, mesh offset |
| Camera | FOV, near/far planes, active flag |
| Skeleton | bone matrices, joint hierarchy |
| Animation | clip index, time, playing state |
| RigidBody | velocity, gravity, grounded flag |
| PlayerController | move speed, turn speed |
| FacingDirection | yaw angle, turn speed |
| FollowTarget | camera-player relationship |

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
├── procedural/
│   └── BuildingGenerator.h  # Procedural mesh generation
├── spatial/
│   └── SpatialGrid.h   # Spatial culling structures
├── ui/
│   ├── FontManager.h   # Font loading
│   └── TextCache.h     # Text texture caching
└── Shader.h            # Shader compilation/linking

shaders/
├── model.vert/frag     # Standard lit shader
├── skinned.vert        # Skeletal animation shader
├── color.vert/frag     # Debug/solid color shader
├── ui.vert/frag        # 2D UI rendering
├── minimap.vert/frag   # Circular minimap
├── minimap_rect.vert/frag  # Building footprints
└── terrain.vert/frag   # Ground plane shader
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
