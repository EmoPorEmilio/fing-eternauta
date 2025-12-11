#pragma once
#include "../ecs/Registry.h"
#include "../ecs/Entity.h"
#include "../ecs/components/MonsterData.h"
#include "../ecs/components/Transform.h"
#include "../ecs/components/Animation.h"
#include "../ecs/components/Renderable.h"
#include "../ecs/components/FacingDirection.h"
#include "../core/AssetManager.h"
#include "../procedural/BuildingGenerator.h"
#include <vector>
#include <random>
#include <cmath>

class MonsterManager {
public:
    // Result of update - tells caller about chase/catch events
    struct UpdateResult {
        bool playerCaught = false;
        bool chaseStarted = false;      // True when monster first detects player
        float distanceToPlayer = 0.0f;  // Distance when chase started (for cinematic duration calc)
    };

    void init(Registry* registry, AssetManager* assetManager) {
        m_registry = registry;
        m_assetManager = assetManager;
    }

    // Spawn monsters procedurally across the grid
    // spawnDensity: 1.0 = one monster per block, 0.5 = half that, etc.
    void spawnAll(float spawnDensity = 0.1f, unsigned int seed = 12345) {
        if (!m_registry || !m_assetManager) return;

        std::mt19937 rng(seed);
        std::uniform_real_distribution<float> spawnChance(0.0f, 1.0f);
        std::uniform_int_distribution<int> streetDir(0, 1);  // 0 = X direction, 1 = Z direction
        std::uniform_real_distribution<float> streetOffset(-4.0f, 4.0f);  // Random offset within street

        LoadedModel& monsterModel = m_assetManager->getModel("monster");

        float offsetX = BuildingGenerator::getGridOffsetX();
        float offsetZ = BuildingGenerator::getGridOffsetZ();

        // Spawn monsters in a subset of the grid (around the center where gameplay happens)
        int spawnRadius = 25;  // Spawn in a 50x50 area around center
        int centerGrid = BuildingGenerator::GRID_SIZE / 2;

        for (int z = centerGrid - spawnRadius; z < centerGrid + spawnRadius; ++z) {
            for (int x = centerGrid - spawnRadius; x < centerGrid + spawnRadius; ++x) {
                if (x < 0 || x >= BuildingGenerator::GRID_SIZE ||
                    z < 0 || z >= BuildingGenerator::GRID_SIZE) continue;

                // Random chance to spawn
                if (spawnChance(rng) > spawnDensity) continue;

                // Calculate street position (in the middle of the street, not on building)
                // Building center is at: offsetX + x * BLOCK_SIZE + BUILDING_WIDTH/2
                // Street runs from (building edge) to (next building edge)
                // Street center X: offsetX + x * BLOCK_SIZE + BUILDING_WIDTH + STREET_WIDTH/2
                // Or between buildings in Z direction

                bool useXStreet = streetDir(rng) == 0;
                glm::vec3 patrolStart, patrolEnd;

                if (useXStreet) {
                    // Patrol along X axis (horizontal street)
                    float streetZ = offsetZ + z * BuildingGenerator::BLOCK_SIZE + BuildingGenerator::BUILDING_DEPTH + BuildingGenerator::STREET_WIDTH / 2.0f;
                    float startX = offsetX + x * BuildingGenerator::BLOCK_SIZE + BuildingGenerator::BUILDING_WIDTH + 1.0f;
                    float endX = offsetX + (x + 1) * BuildingGenerator::BLOCK_SIZE - 1.0f;

                    patrolStart = glm::vec3(startX, 0.0f, streetZ + streetOffset(rng));
                    patrolEnd = glm::vec3(endX, 0.0f, streetZ + streetOffset(rng));
                } else {
                    // Patrol along Z axis (vertical street)
                    float streetX = offsetX + x * BuildingGenerator::BLOCK_SIZE + BuildingGenerator::BUILDING_WIDTH + BuildingGenerator::STREET_WIDTH / 2.0f;
                    float startZ = offsetZ + z * BuildingGenerator::BLOCK_SIZE + BuildingGenerator::BUILDING_DEPTH + 1.0f;
                    float endZ = offsetZ + (z + 1) * BuildingGenerator::BLOCK_SIZE - 1.0f;

                    patrolStart = glm::vec3(streetX + streetOffset(rng), 0.0f, startZ);
                    patrolEnd = glm::vec3(streetX + streetOffset(rng), 0.0f, endZ);
                }

                Entity monster = spawnMonster(monsterModel, patrolStart, patrolEnd, x, z);
                m_monsters.push_back(monster);
            }
        }

        std::cout << "MonsterManager: Spawned " << m_monsters.size() << " monsters" << std::endl;
    }

    // Update all monsters - returns true if player was caught
    UpdateResult update(float dt, const glm::vec3& playerPos) {
        UpdateResult result;

        m_registry->forEachMonster([&](Entity entity, Transform& transform, MonsterData& data, Animation* anim) {
            updateMonster(entity, transform, data, anim, dt, playerPos, result);
        });

        return result;
    }

    // Get visible monster positions for minimap rendering (only unculled monsters)
    std::vector<glm::vec3> getPositions() const {
        std::vector<glm::vec3> positions;
        positions.reserve(m_monsters.size());

        for (Entity e : m_monsters) {
            if (m_registry->isAlive(e)) {
                auto* r = m_registry->getRenderable(e);
                if (r && r->visible) {
                    auto* t = m_registry->getTransform(e);
                    if (t) {
                        positions.push_back(t->position);
                    }
                }
            }
        }

        return positions;
    }

    // Get monster count
    size_t getMonsterCount() const { return m_monsters.size(); }

    // Get monster entity list (for shadow rendering)
    const std::vector<Entity>& getMonsterEntities() const { return m_monsters; }

    // Reset all monsters to their initial patrol state
    void resetAll() {
        for (Entity e : m_monsters) {
            if (!m_registry->isAlive(e)) continue;

            auto* transform = m_registry->getTransform(e);
            auto* data = m_registry->getMonsterData(e);
            auto* anim = m_registry->getAnimation(e);

            if (transform && data) {
                // Reset position to patrol midpoint
                transform->position = (data->patrolStart + data->patrolEnd) * 0.5f;
                transform->position.y = 0.005f;

                // Reset state to patrol
                data->state = MonsterData::State::Patrol;
                data->movingToEnd = true;

                // Reset animation speed
                if (anim) {
                    anim->speedMultiplier = 1.0f;
                }
            }
        }
    }

private:
    Registry* m_registry = nullptr;
    AssetManager* m_assetManager = nullptr;
    std::vector<Entity> m_monsters;

    Entity spawnMonster(LoadedModel& model, const glm::vec3& patrolStart, const glm::vec3& patrolEnd,
                        int gridX, int gridZ) {
        Entity monster = m_registry->create();

        // Transform - start at patrol midpoint
        Transform transform;
        transform.position = (patrolStart + patrolEnd) * 0.5f;
        transform.position.y = 0.005f;  // Slightly above ground
        transform.scale = glm::vec3(4.0f);

        // Rotation: 90 degrees around X to stand upright, 180 around Y to face correctly
        glm::quat rotX = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat rotY = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        transform.rotation = rotY * rotX;

        m_registry->addTransform(monster, transform);

        // MeshGroup
        MeshGroup meshGroup;
        meshGroup.meshes = model.meshGroup.meshes;
        m_registry->addMeshGroup(monster, std::move(meshGroup));

        // Renderable
        Renderable renderable;
        renderable.shader = ShaderType::Skinned;
        renderable.meshOffset = glm::vec3(0.0f);
        m_registry->addRenderable(monster, renderable);

        // FacingDirection
        FacingDirection facing;
        facing.yaw = 0.0f;
        m_registry->addFacingDirection(monster, facing);

        // Skeleton and Animation
        if (model.skeleton) {
            Skeleton skeleton = *model.skeleton;
            m_registry->addSkeleton(monster, std::move(skeleton));

            Animation anim;
            anim.clipIndex = 0;
            anim.playing = true;
            anim.time = static_cast<float>(std::rand() % 1000) / 1000.0f * 2.0f;  // Random start time
            anim.speedMultiplier = 1.0f;
            anim.clips = model.clips;
            m_registry->addAnimation(monster, anim);
        }

        // MonsterData - patrol info
        MonsterData data;
        data.state = MonsterData::State::Patrol;
        data.patrolStart = patrolStart;
        data.patrolEnd = patrolEnd;
        data.movingToEnd = true;
        data.gridX = gridX;
        data.gridZ = gridZ;
        m_registry->addMonsterData(monster, data);

        return monster;
    }

    // Render distance: 2 building blocks
    static constexpr float RENDER_DISTANCE = 2.0f * BuildingGenerator::BLOCK_SIZE;

    void updateMonster(Entity entity, Transform& transform, MonsterData& data, Animation* anim,
                       float dt, const glm::vec3& playerPos, UpdateResult& result) {
        float distToPlayer = glm::distance(glm::vec2(transform.position.x, transform.position.z),
                                           glm::vec2(playerPos.x, playerPos.z));

        // Update visibility based on distance (for culling)
        auto* renderable = m_registry->getRenderable(entity);
        if (renderable) {
            renderable->visible = (distToPlayer < RENDER_DISTANCE);
        }

        // State transitions
        if (data.state == MonsterData::State::Patrol) {
            // Check if player is within detection range
            if (distToPlayer < MonsterData::DETECTION_RADIUS) {
                data.state = MonsterData::State::Chase;
                if (anim) anim->speedMultiplier = 10.0f;  // Frenzy mode

                // Signal that chase started - trigger cinematic
                result.chaseStarted = true;
                result.distanceToPlayer = distToPlayer;
                return;  // Don't move this frame - let cinematic take over
            }
        } else if (data.state == MonsterData::State::Chase) {
            // Check if player caught
            if (distToPlayer < MonsterData::CATCH_RADIUS) {
                result.playerCaught = true;
                return;
            }
            // Check if player escaped (only during normal gameplay, not cinematic)
            if (distToPlayer > MonsterData::ESCAPE_RADIUS) {
                data.state = MonsterData::State::Patrol;
                if (anim) anim->speedMultiplier = 1.0f;  // Normal speed
            }
        }

        // Movement based on state
        if (data.state == MonsterData::State::Patrol) {
            updatePatrol(transform, data, dt);
        } else {
            updateChase(transform, data, dt, playerPos);
        }
    }

    void updatePatrol(Transform& transform, MonsterData& data, float dt) {
        // Determine target waypoint
        glm::vec3 target = data.movingToEnd ? data.patrolEnd : data.patrolStart;

        // Calculate direction to target
        glm::vec3 toTarget = target - transform.position;
        toTarget.y = 0.0f;  // Keep on ground plane

        float dist = glm::length(toTarget);

        // If close to target, flip direction
        if (dist < 0.5f) {
            data.movingToEnd = !data.movingToEnd;
            return;
        }

        // Move toward target
        glm::vec3 direction = glm::normalize(toTarget);
        transform.position += direction * MonsterData::PATROL_SPEED * dt;
        transform.position.y = 0.005f;  // Keep on ground

        // Rotate to face movement direction
        rotateToFace(transform, direction, dt);
    }

    void updateChase(Transform& transform, MonsterData& data, float dt, const glm::vec3& playerPos) {
        // Direction to player
        glm::vec3 toPlayer = playerPos - transform.position;
        toPlayer.y = 0.0f;

        float dist = glm::length(toPlayer);
        if (dist < 0.1f) return;

        glm::vec3 direction = glm::normalize(toPlayer);

        // Move toward player at chase speed
        transform.position += direction * MonsterData::CHASE_SPEED * dt;
        transform.position.y = 0.005f;

        // Rotate to face player
        rotateToFace(transform, direction, dt);
    }

    void rotateToFace(Transform& transform, const glm::vec3& direction, float dt) {
        // The monster model needs specific rotation to stand upright
        // Base rotation: 90 degrees X to stand up

        // Calculate target yaw from direction (no 180 offset - face movement direction)
        float targetYaw = std::atan2(direction.x, direction.z);

        // Build rotation: first X 90 to stand up, then Y to face direction
        glm::quat rotX = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::quat rotY = glm::angleAxis(targetYaw, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::quat targetRot = rotY * rotX;

        // Smooth rotation
        transform.rotation = glm::slerp(transform.rotation, targetRot, MonsterData::TURN_SPEED * dt);
    }
};
