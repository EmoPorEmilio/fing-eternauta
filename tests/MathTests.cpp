/**
 * @file MathTests.cpp
 * @brief Comprehensive unit tests for all math-related operations in the ECS engine
 *
 * ============================================================================
 * TEST SUITES OVERVIEW
 * ============================================================================
 *
 * 1. EntityID Tests
 *    - Tests the bit manipulation for entity ID encoding/decoding
 *    - Verifies index extraction (lower 20 bits)
 *    - Verifies generation extraction (upper 12 bits)
 *    - Tests boundary conditions and invalid entity handling
 *
 * 2. TransformComponent Tests
 *    - Tests model matrix computation from position/rotation/scale
 *    - Verifies direction vector calculations (forward, right, up)
 *    - Tests dirty flag behavior
 *    - Validates quaternion-based rotation
 *
 * 3. LODComponent Tests
 *    - Tests distance-based LOD level calculation
 *    - Verifies LOD bias application
 *    - Tests boundary conditions at LOD transition distances
 *
 * 4. CameraComponent Tests
 *    - Tests direction vector computation from yaw/pitch
 *    - Verifies view matrix calculation
 *    - Tests projection matrix (perspective and orthographic)
 *    - Validates pitch clamping behavior
 *
 * ============================================================================
 * MATHEMATICAL FORMULAS BEING TESTED
 * ============================================================================
 *
 * Entity ID Encoding:
 *   ID = (generation & 0xFFF) << 20 | (index & 0xFFFFF)
 *   Index = ID & 0xFFFFF (lower 20 bits)
 *   Generation = (ID >> 20) & 0xFFF (upper 12 bits)
 *
 * Model Matrix:
 *   M = Translate(position) * Rotate(quaternion) * Scale(scale)
 *
 * Direction from Yaw/Pitch:
 *   front.x = cos(yaw) * cos(pitch)
 *   front.y = sin(pitch)
 *   front.z = sin(yaw) * cos(pitch)
 *   right = normalize(cross(front, worldUp))
 *   up = normalize(cross(right, front))
 *
 * LOD Selection:
 *   if (distance + bias <= highDistance)    -> HIGH
 *   else if (distance + bias <= medDistance) -> MEDIUM
 *   else                                      -> LOW
 *
 * ============================================================================
 * BUILD INSTRUCTIONS
 * ============================================================================
 *
 * Compile with:
 *   cl /EHsc /std:c++17 /I..\libraries\glm tests\MathTests.cpp /Fe:MathTests.exe
 *
 * Or via MSBuild with the test project configuration.
 *
 * ============================================================================
 */

#include "MathTestFramework.h"
#include "../ecs/Entity.h"
#include "../components/TransformComponent.h"
#include "../components/LODComponent.h"
#include "../components/CameraComponent.h"
#include "../Prism.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

using namespace ecs;

// ============================================================================
// ENTITY ID TESTS
// ============================================================================
// These tests verify the bit manipulation used to encode entity IDs.
// Entity IDs pack an index (20 bits) and generation (12 bits) into 32 bits.
// This allows ~1 million entities with 4096 generations for reuse detection.
// ============================================================================

/**
 * @test EntityID_MakeAndExtract_Index
 * @brief Verify that makeEntityID correctly encodes the index and getEntityIndex extracts it
 *
 * Formula: index = ID & 0xFFFFF (lower 20 bits)
 *
 * Test cases:
 *   - Zero index
 *   - Small index
 *   - Maximum index (2^20 - 1 = 1,048,575)
 */
TEST(EntityID, MakeAndExtract_Index) {
    // Case 1: Zero index
    EntityID id1 = makeEntityID(0, 0);
    ASSERT_EQ(getEntityIndex(id1), 0u);

    // Case 2: Typical index
    EntityID id2 = makeEntityID(12345, 0);
    ASSERT_EQ(getEntityIndex(id2), 12345u);

    // Case 3: Maximum valid index (20 bits = 1,048,575)
    EntityID id3 = makeEntityID(0xFFFFF, 0);
    ASSERT_EQ(getEntityIndex(id3), 0xFFFFFu);

    // Case 4: Index with generation (ensure generation doesn't affect index)
    EntityID id4 = makeEntityID(99999, 500);
    ASSERT_EQ(getEntityIndex(id4), 99999u);
}

/**
 * @test EntityID_MakeAndExtract_Generation
 * @brief Verify that makeEntityID correctly encodes generation and getEntityGeneration extracts it
 *
 * Formula: generation = (ID >> 20) & 0xFFF (upper 12 bits)
 *
 * Test cases:
 *   - Zero generation
 *   - Typical generation
 *   - Maximum generation (2^12 - 1 = 4095)
 */
TEST(EntityID, MakeAndExtract_Generation) {
    // Case 1: Zero generation
    EntityID id1 = makeEntityID(100, 0);
    ASSERT_EQ(getEntityGeneration(id1), 0u);

    // Case 2: Typical generation
    EntityID id2 = makeEntityID(100, 42);
    ASSERT_EQ(getEntityGeneration(id2), 42u);

    // Case 3: Maximum valid generation (12 bits = 4095)
    EntityID id3 = makeEntityID(100, 0xFFF);
    ASSERT_EQ(getEntityGeneration(id3), 0xFFFu);

    // Case 4: Different indices with same generation
    EntityID id4a = makeEntityID(0, 100);
    EntityID id4b = makeEntityID(1000, 100);
    ASSERT_EQ(getEntityGeneration(id4a), 100u);
    ASSERT_EQ(getEntityGeneration(id4b), 100u);
}

/**
 * @test EntityID_BitMasking
 * @brief Verify that overflow is properly masked (only lower bits used)
 *
 * When index > 0xFFFFF or generation > 0xFFF, only the valid bits should be used.
 * This prevents undefined behavior and ensures predictable wraparound.
 */
TEST(EntityID, BitMasking) {
    // Index overflow: 0x100000 should wrap to 0
    EntityID id1 = makeEntityID(0x100000, 0);
    ASSERT_EQ(getEntityIndex(id1), 0u);

    // Index overflow: 0x100001 should wrap to 1
    EntityID id2 = makeEntityID(0x100001, 0);
    ASSERT_EQ(getEntityIndex(id2), 1u);

    // Generation overflow: 0x1000 should wrap to 0
    EntityID id3 = makeEntityID(0, 0x1000);
    ASSERT_EQ(getEntityGeneration(id3), 0u);

    // Generation overflow: 0x1001 should wrap to 1
    EntityID id4 = makeEntityID(0, 0x1001);
    ASSERT_EQ(getEntityGeneration(id4), 1u);
}

/**
 * @test EntityID_InvalidEntity
 * @brief Verify INVALID_ENTITY constant behavior
 *
 * INVALID_ENTITY should be the maximum uint32_t value (0xFFFFFFFF).
 * Entity::isValid() should return false for this value.
 */
TEST(EntityID, InvalidEntity) {
    // INVALID_ENTITY should be max uint32
    ASSERT_EQ(INVALID_ENTITY, 0xFFFFFFFFu);

    // Default Entity should be invalid
    Entity e1;
    ASSERT_FALSE(e1.isValid());

    // Entity with INVALID_ENTITY should be invalid
    Entity e2(INVALID_ENTITY);
    ASSERT_FALSE(e2.isValid());

    // Valid entity should be valid
    Entity e3(makeEntityID(0, 0));
    ASSERT_TRUE(e3.isValid());
}

/**
 * @test EntityID_Comparison
 * @brief Verify Entity comparison operators
 *
 * Entities should compare by their raw ID values.
 */
TEST(EntityID, Comparison) {
    Entity e1(makeEntityID(100, 1));
    Entity e2(makeEntityID(100, 1));
    Entity e3(makeEntityID(100, 2));
    Entity e4(makeEntityID(200, 1));

    // Same ID should be equal
    ASSERT_TRUE(e1 == e2);
    ASSERT_FALSE(e1 != e2);

    // Different generation should not be equal
    ASSERT_FALSE(e1 == e3);
    ASSERT_TRUE(e1 != e3);

    // Different index should not be equal
    ASSERT_FALSE(e1 == e4);

    // Less-than should work for ordering
    ASSERT_TRUE(e1 < e3);  // Lower generation
    ASSERT_TRUE(e1 < e4);  // Lower index (since generation << 20)
}

// ============================================================================
// TRANSFORM COMPONENT TESTS
// ============================================================================
// These tests verify the model matrix computation and direction vector math.
// The model matrix is: M = Translate * Rotate * Scale
// Direction vectors are computed by rotating basis vectors by the quaternion.
// ============================================================================

/**
 * @test Transform_ModelMatrix_Translation
 * @brief Verify model matrix correctly applies translation
 *
 * For translation-only transform:
 *   M = | 1 0 0 tx |
 *       | 0 1 0 ty |
 *       | 0 0 1 tz |
 *       | 0 0 0 1  |
 */
TEST(Transform, ModelMatrix_Translation) {
    TransformComponent t;
    t.position = glm::vec3(10.0f, 20.0f, 30.0f);
    t.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity
    t.scale = glm::vec3(1.0f);
    t.dirty = true;

    t.updateModelMatrix();

    // Check translation column
    ASSERT_NEAR(t.modelMatrix[3][0], 10.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[3][1], 20.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[3][2], 30.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[3][3], 1.0f, 0.0001f);

    // Check identity rotation/scale (diagonal should be 1)
    ASSERT_NEAR(t.modelMatrix[0][0], 1.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[1][1], 1.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[2][2], 1.0f, 0.0001f);
}

/**
 * @test Transform_ModelMatrix_Scale
 * @brief Verify model matrix correctly applies uniform and non-uniform scale
 *
 * For scale-only transform:
 *   M = | sx 0  0  0 |
 *       | 0  sy 0  0 |
 *       | 0  0  sz 0 |
 *       | 0  0  0  1 |
 */
TEST(Transform, ModelMatrix_Scale) {
    TransformComponent t;
    t.position = glm::vec3(0.0f);
    t.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity
    t.scale = glm::vec3(2.0f, 3.0f, 4.0f);
    t.dirty = true;

    t.updateModelMatrix();

    // Check scale on diagonal
    ASSERT_NEAR(t.modelMatrix[0][0], 2.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[1][1], 3.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[2][2], 4.0f, 0.0001f);

    // No translation
    ASSERT_NEAR(t.modelMatrix[3][0], 0.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[3][1], 0.0f, 0.0001f);
    ASSERT_NEAR(t.modelMatrix[3][2], 0.0f, 0.0001f);
}

/**
 * @test Transform_ModelMatrix_Rotation90Y
 * @brief Verify model matrix for 90-degree rotation around Y axis
 *
 * Rotating 90 degrees around Y:
 *   - X axis (1,0,0) maps to Z axis (0,0,1)
 *   - Z axis (0,0,1) maps to -X axis (-1,0,0)
 */
TEST(Transform, ModelMatrix_Rotation90Y) {
    TransformComponent t;
    t.position = glm::vec3(0.0f);
    t.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    t.scale = glm::vec3(1.0f);
    t.dirty = true;

    t.updateModelMatrix();

    // Transform basis vectors
    glm::vec4 xAxis = t.modelMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 zAxis = t.modelMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

    // X should become Z
    ASSERT_NEAR(xAxis.x, 0.0f, 0.0001f);
    ASSERT_NEAR(xAxis.z, -1.0f, 0.0001f);  // Note: right-hand rule

    // Z should become -X
    ASSERT_NEAR(zAxis.x, 1.0f, 0.0001f);
    ASSERT_NEAR(zAxis.z, 0.0f, 0.0001f);
}

/**
 * @test Transform_DirectionVectors_Identity
 * @brief Verify direction vectors for identity rotation
 *
 * For identity quaternion:
 *   forward = (0, 0, -1)  // Looking down -Z
 *   right   = (1, 0, 0)
 *   up      = (0, 1, 0)
 */
TEST(Transform, DirectionVectors_Identity) {
    TransformComponent t;
    t.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity

    glm::vec3 forward = t.forward();
    glm::vec3 right = t.right();
    glm::vec3 up = t.up();

    ASSERT_VEC3_NEAR(forward, glm::vec3(0.0f, 0.0f, -1.0f), 0.0001f);
    ASSERT_VEC3_NEAR(right, glm::vec3(1.0f, 0.0f, 0.0f), 0.0001f);
    ASSERT_VEC3_NEAR(up, glm::vec3(0.0f, 1.0f, 0.0f), 0.0001f);
}

/**
 * @test Transform_DirectionVectors_Rotated
 * @brief Verify direction vectors after 90-degree Y rotation
 *
 * After 90-degree Y rotation (right-hand rule):
 *   Rotating (0,0,-1) by +90° around Y gives (-1,0,0)
 *   Rotation matrix for +90° Y: [0 0 1; 0 1 0; -1 0 0]
 *   Applied to (0,0,-1): x'=1*(-1)=-1, y'=0, z'=0
 */
TEST(Transform, DirectionVectors_Rotated) {
    TransformComponent t;
    t.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 forward = t.forward();

    // After 90-deg Y rotation with right-hand rule:
    // forward (0,0,-1) becomes (-1,0,0)
    ASSERT_NEAR(forward.x, -1.0f, 0.001f);
    ASSERT_NEAR(forward.y, 0.0f, 0.001f);
    ASSERT_NEAR(forward.z, 0.0f, 0.001f);
}

/**
 * @test Transform_DirtyFlag
 * @brief Verify dirty flag behavior
 *
 * The dirty flag should:
 *   - Be set when transform properties change
 *   - Be cleared after updateModelMatrix()
 *   - Prevent redundant matrix computation when clean
 */
TEST(Transform, DirtyFlag) {
    TransformComponent t;

    // Starts dirty
    ASSERT_TRUE(t.dirty);

    // Update clears dirty
    t.updateModelMatrix();
    ASSERT_FALSE(t.dirty);

    // setPosition makes dirty
    t.setPosition(glm::vec3(5.0f, 0.0f, 0.0f));
    ASSERT_TRUE(t.dirty);

    t.updateModelMatrix();
    ASSERT_FALSE(t.dirty);

    // setRotation makes dirty
    t.setRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    ASSERT_TRUE(t.dirty);

    t.updateModelMatrix();
    ASSERT_FALSE(t.dirty);

    // setScale makes dirty
    t.setScale(glm::vec3(2.0f));
    ASSERT_TRUE(t.dirty);
}

/**
 * @test Transform_CombinedTRS
 * @brief Verify combined Translation-Rotation-Scale matrix
 *
 * Tests that the order T*R*S is correctly applied.
 * A point at origin with scale 2, rotation 90Y, translation (10,0,0)
 * should first scale, then rotate, then translate.
 */
TEST(Transform, CombinedTRS) {
    TransformComponent t;
    t.position = glm::vec3(10.0f, 0.0f, 0.0f);
    t.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    t.scale = glm::vec3(2.0f, 2.0f, 2.0f);
    t.dirty = true;

    t.updateModelMatrix();

    // Transform a point at (1, 0, 0):
    // 1. Scale: (1,0,0) * 2 = (2,0,0)
    // 2. Rotate 90Y: (2,0,0) -> (0,0,-2)
    // 3. Translate: (0,0,-2) + (10,0,0) = (10,0,-2)
    glm::vec4 point = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 result = t.modelMatrix * point;

    ASSERT_NEAR(result.x, 10.0f, 0.001f);
    ASSERT_NEAR(result.y, 0.0f, 0.001f);
    ASSERT_NEAR(result.z, -2.0f, 0.001f);
}

// ============================================================================
// LOD COMPONENT TESTS
// ============================================================================
// These tests verify the Level of Detail selection algorithm.
// LOD is selected based on distance with optional bias adjustment.
// ============================================================================

/**
 * @test LOD_CalculateLOD_HighDistance
 * @brief Verify HIGH LOD is selected for close distances
 *
 * When distance <= highDistance, should return HIGH LOD.
 */
TEST(LOD, CalculateLOD_HighDistance) {
    LODComponent lod;
    lod.highDistance = 50.0f;
    lod.mediumDistance = 150.0f;

    // Well within HIGH range
    ASSERT_TRUE(lod.calculateLOD(0.0f) == LODLevel::HIGH);
    ASSERT_TRUE(lod.calculateLOD(25.0f) == LODLevel::HIGH);
    ASSERT_TRUE(lod.calculateLOD(49.9f) == LODLevel::HIGH);

    // Exactly at boundary (inclusive)
    ASSERT_TRUE(lod.calculateLOD(50.0f) == LODLevel::HIGH);
}

/**
 * @test LOD_CalculateLOD_MediumDistance
 * @brief Verify MEDIUM LOD is selected for mid-range distances
 *
 * When highDistance < distance <= mediumDistance, should return MEDIUM LOD.
 */
TEST(LOD, CalculateLOD_MediumDistance) {
    LODComponent lod;
    lod.highDistance = 50.0f;
    lod.mediumDistance = 150.0f;

    // Just past HIGH boundary
    ASSERT_TRUE(lod.calculateLOD(50.1f) == LODLevel::MEDIUM);

    // Mid-range
    ASSERT_TRUE(lod.calculateLOD(100.0f) == LODLevel::MEDIUM);

    // Just before LOW boundary
    ASSERT_TRUE(lod.calculateLOD(149.9f) == LODLevel::MEDIUM);

    // Exactly at boundary (inclusive)
    ASSERT_TRUE(lod.calculateLOD(150.0f) == LODLevel::MEDIUM);
}

/**
 * @test LOD_CalculateLOD_LowDistance
 * @brief Verify LOW LOD is selected for far distances
 *
 * When distance > mediumDistance, should return LOW LOD.
 */
TEST(LOD, CalculateLOD_LowDistance) {
    LODComponent lod;
    lod.highDistance = 50.0f;
    lod.mediumDistance = 150.0f;

    // Just past MEDIUM boundary
    ASSERT_TRUE(lod.calculateLOD(150.1f) == LODLevel::LOW);

    // Far away
    ASSERT_TRUE(lod.calculateLOD(500.0f) == LODLevel::LOW);
    ASSERT_TRUE(lod.calculateLOD(10000.0f) == LODLevel::LOW);
}

/**
 * @test LOD_Bias_Positive
 * @brief Verify positive LOD bias forces lower detail
 *
 * Positive bias increases effective distance, causing earlier transition
 * to lower LOD levels.
 */
TEST(LOD, Bias_Positive) {
    LODComponent lod;
    lod.highDistance = 50.0f;
    lod.mediumDistance = 150.0f;
    lod.lodBias = 25.0f;  // +25 bias

    // At distance 30, effective = 30 + 25 = 55, so MEDIUM
    ASSERT_TRUE(lod.calculateLOD(30.0f) == LODLevel::MEDIUM);

    // At distance 126, effective = 126 + 25 = 151, so LOW
    ASSERT_TRUE(lod.calculateLOD(126.0f) == LODLevel::LOW);
}

/**
 * @test LOD_Bias_Negative
 * @brief Verify negative LOD bias forces higher detail
 *
 * Negative bias decreases effective distance, causing later transition
 * to lower LOD levels.
 */
TEST(LOD, Bias_Negative) {
    LODComponent lod;
    lod.highDistance = 50.0f;
    lod.mediumDistance = 150.0f;
    lod.lodBias = -25.0f;  // -25 bias

    // At distance 70, effective = 70 - 25 = 45, so HIGH
    ASSERT_TRUE(lod.calculateLOD(70.0f) == LODLevel::HIGH);

    // At distance 170, effective = 170 - 25 = 145, so MEDIUM
    ASSERT_TRUE(lod.calculateLOD(170.0f) == LODLevel::MEDIUM);
}

/**
 * @test LOD_UpdateLOD
 * @brief Verify updateLOD() uses cached distanceToCamera
 */
TEST(LOD, UpdateLOD) {
    LODComponent lod;
    lod.highDistance = 50.0f;
    lod.mediumDistance = 150.0f;

    // Set distance and update
    lod.distanceToCamera = 75.0f;
    lod.updateLOD();
    ASSERT_TRUE(lod.currentLevel == LODLevel::MEDIUM);

    // Change distance and update again
    lod.distanceToCamera = 25.0f;
    lod.updateLOD();
    ASSERT_TRUE(lod.currentLevel == LODLevel::HIGH);
}

// ============================================================================
// CAMERA COMPONENT TESTS
// ============================================================================
// These tests verify camera math including:
// - Direction vector computation from Euler angles
// - View matrix calculation
// - Projection matrix calculation (perspective and orthographic)
// ============================================================================

/**
 * @test Camera_DirectionVectors_DefaultYaw
 * @brief Verify direction vectors for default yaw (-90 degrees)
 *
 * At yaw=-90, pitch=0:
 *   front.x = cos(-90) * cos(0) = 0
 *   front.y = sin(0) = 0
 *   front.z = sin(-90) * cos(0) = -1
 *   front = (0, 0, -1), i.e., looking down -Z
 */
TEST(Camera, DirectionVectors_DefaultYaw) {
    CameraComponent cam;
    cam.yaw = -90.0f;
    cam.pitch = 0.0f;
    cam.updateVectors();

    ASSERT_VEC3_NEAR(cam.front, glm::vec3(0.0f, 0.0f, -1.0f), 0.0001f);
    ASSERT_VEC3_NEAR(cam.right, glm::vec3(1.0f, 0.0f, 0.0f), 0.0001f);
    ASSERT_VEC3_NEAR(cam.up, glm::vec3(0.0f, 1.0f, 0.0f), 0.0001f);
}

/**
 * @test Camera_DirectionVectors_Yaw0
 * @brief Verify direction vectors for yaw=0 (looking down +X)
 *
 * At yaw=0, pitch=0:
 *   front.x = cos(0) * cos(0) = 1
 *   front.y = sin(0) = 0
 *   front.z = sin(0) * cos(0) = 0
 *   front = (1, 0, 0)
 */
TEST(Camera, DirectionVectors_Yaw0) {
    CameraComponent cam;
    cam.yaw = 0.0f;
    cam.pitch = 0.0f;
    cam.updateVectors();

    ASSERT_VEC3_NEAR(cam.front, glm::vec3(1.0f, 0.0f, 0.0f), 0.0001f);
}

/**
 * @test Camera_DirectionVectors_Pitch45
 * @brief Verify direction vectors for 45-degree pitch (looking up)
 *
 * At yaw=-90, pitch=45:
 *   front.x = cos(-90) * cos(45) = 0
 *   front.y = sin(45) = 0.707...
 *   front.z = sin(-90) * cos(45) = -0.707...
 */
TEST(Camera, DirectionVectors_Pitch45) {
    CameraComponent cam;
    cam.yaw = -90.0f;
    cam.pitch = 45.0f;
    cam.updateVectors();

    float cos45 = std::cos(glm::radians(45.0f));
    float sin45 = std::sin(glm::radians(45.0f));

    ASSERT_NEAR(cam.front.x, 0.0f, 0.0001f);
    ASSERT_NEAR(cam.front.y, sin45, 0.0001f);
    ASSERT_NEAR(cam.front.z, -cos45, 0.0001f);
}

/**
 * @test Camera_PitchClamping
 * @brief Verify pitch is clamped to prevent gimbal lock
 *
 * Pitch should be clamped to [minPitch, maxPitch] when constrainPitch is true.
 */
TEST(Camera, PitchClamping) {
    CameraComponent cam;
    cam.yaw = -90.0f;
    cam.pitch = 0.0f;
    cam.minPitch = -89.0f;
    cam.maxPitch = 89.0f;
    cam.constrainPitch = true;

    // Apply extreme positive mouse input
    cam.applyMouseInput(0.0f, 1000.0f);

    // Pitch should be clamped to maxPitch
    ASSERT_LE(cam.pitch, cam.maxPitch);
    ASSERT_NEAR(cam.pitch, 89.0f, 0.001f);

    // Apply extreme negative mouse input
    cam.applyMouseInput(0.0f, -2000.0f);

    // Pitch should be clamped to minPitch
    ASSERT_GE(cam.pitch, cam.minPitch);
    ASSERT_NEAR(cam.pitch, -89.0f, 0.001f);
}

/**
 * @test Camera_ViewMatrix
 * @brief Verify view matrix calculation
 *
 * The view matrix transforms world coordinates to camera space.
 * For a camera at origin looking down -Z:
 *   viewMatrix should be identity (approximately)
 */
TEST(Camera, ViewMatrix) {
    CameraComponent cam;
    cam.yaw = -90.0f;
    cam.pitch = 0.0f;
    cam.updateVectors();

    glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);
    glm::mat4 view = cam.calculateViewMatrix(cameraPos);

    // A point at (0, 0, -5) in world space should be at (0, 0, -5) in view space
    // (since camera is at origin looking down -Z)
    glm::vec4 worldPoint(0.0f, 0.0f, -5.0f, 1.0f);
    glm::vec4 viewPoint = view * worldPoint;

    ASSERT_NEAR(viewPoint.x, 0.0f, 0.0001f);
    ASSERT_NEAR(viewPoint.y, 0.0f, 0.0001f);
    ASSERT_NEAR(viewPoint.z, -5.0f, 0.0001f);
}

/**
 * @test Camera_ViewMatrix_Translated
 * @brief Verify view matrix for translated camera
 *
 * A camera at (10, 0, 0) looking down -Z:
 * A point at (10, 0, -5) should appear at (0, 0, -5) in view space.
 */
TEST(Camera, ViewMatrix_Translated) {
    CameraComponent cam;
    cam.yaw = -90.0f;
    cam.pitch = 0.0f;
    cam.updateVectors();

    glm::vec3 cameraPos(10.0f, 0.0f, 0.0f);
    glm::mat4 view = cam.calculateViewMatrix(cameraPos);

    // World point directly in front of camera
    glm::vec4 worldPoint(10.0f, 0.0f, -5.0f, 1.0f);
    glm::vec4 viewPoint = view * worldPoint;

    ASSERT_NEAR(viewPoint.x, 0.0f, 0.0001f);
    ASSERT_NEAR(viewPoint.y, 0.0f, 0.0001f);
    ASSERT_NEAR(viewPoint.z, -5.0f, 0.0001f);
}

/**
 * @test Camera_PerspectiveProjection
 * @brief Verify perspective projection matrix
 *
 * Tests that:
 *   - Points at the near plane are preserved
 *   - Aspect ratio is correctly applied
 */
TEST(Camera, PerspectiveProjection) {
    CameraComponent cam;
    cam.projection = ProjectionType::PERSPECTIVE;
    cam.fov = 90.0f;  // 90 degree FOV for easy math
    cam.aspectRatio = 1.0f;  // Square viewport
    cam.nearPlane = 1.0f;
    cam.farPlane = 100.0f;

    glm::mat4 proj = cam.calculateProjectionMatrix();

    // At 90 degree FOV, tan(45) = 1, so points at edge of near plane
    // should map to edge of NDC
    // Point at near plane, at the edge of view: (1, 1, -1, 1)
    glm::vec4 point(1.0f, 1.0f, -1.0f, 1.0f);
    glm::vec4 projected = proj * point;
    glm::vec3 ndc = glm::vec3(projected) / projected.w;

    // Should be at edge of NDC space [-1, 1]
    ASSERT_NEAR(std::abs(ndc.x), 1.0f, 0.01f);
    ASSERT_NEAR(std::abs(ndc.y), 1.0f, 0.01f);
}

/**
 * @test Camera_OrthographicProjection
 * @brief Verify orthographic projection matrix
 *
 * In orthographic projection, objects maintain size regardless of distance.
 */
TEST(Camera, OrthographicProjection) {
    CameraComponent cam;
    cam.projection = ProjectionType::ORTHOGRAPHIC;
    cam.orthoSize = 10.0f;  // Half-height = 10
    cam.aspectRatio = 2.0f;  // Width = 2 * height
    cam.nearPlane = 0.1f;
    cam.farPlane = 100.0f;

    glm::mat4 proj = cam.calculateProjectionMatrix();

    // Point at (20, 10, -50) should map to edge of NDC
    // orthoSize=10 means height range is [-10, 10]
    // aspect=2 means width range is [-20, 20]
    glm::vec4 edgePoint(20.0f, 10.0f, -50.0f, 1.0f);
    glm::vec4 projected = proj * edgePoint;
    glm::vec3 ndc = glm::vec3(projected) / projected.w;

    ASSERT_NEAR(ndc.x, 1.0f, 0.0001f);  // At right edge
    ASSERT_NEAR(ndc.y, 1.0f, 0.0001f);  // At top edge
}

/**
 * @test Camera_MouseSensitivity
 * @brief Verify mouse sensitivity is applied correctly
 */
TEST(Camera, MouseSensitivity) {
    CameraComponent cam;
    cam.yaw = 0.0f;
    cam.pitch = 0.0f;
    cam.mouseSensitivity = 0.5f;
    cam.constrainPitch = false;

    // Apply mouse input
    cam.applyMouseInput(10.0f, 20.0f);

    // Yaw should change by 10 * 0.5 = 5
    ASSERT_NEAR(cam.yaw, 5.0f, 0.0001f);

    // Pitch should change by 20 * 0.5 = 10
    ASSERT_NEAR(cam.pitch, 10.0f, 0.0001f);
}

// ============================================================================
// MAIN - Run All Tests
// ============================================================================

int main() {
    return test::TestRunner::runAll();
}
