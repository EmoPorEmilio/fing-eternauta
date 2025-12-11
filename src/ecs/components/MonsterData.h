#pragma once
#include <glm/glm.hpp>

// Monster AI state and patrol data
struct MonsterData {
    enum class State {
        Patrol,  // Walking between patrol points
        Chase    // Chasing the player (frenzy mode)
    };

    State state = State::Patrol;

    // Patrol waypoints (street endpoints in world space)
    glm::vec3 patrolStart{0.0f};
    glm::vec3 patrolEnd{0.0f};
    bool movingToEnd = true;  // Direction flag: true = toward patrolEnd

    // Grid cell this monster belongs to (for debugging/culling)
    int gridX = 0;
    int gridZ = 0;

    // Constants for behavior
    static constexpr float DETECTION_RADIUS = 8.0f;    // Distance at which monster detects player (~half street block)
    static constexpr float CATCH_RADIUS = 1.2f;        // Distance at which monster catches player
    static constexpr float PATROL_SPEED = 1.5f;        // Walking speed during patrol
    static constexpr float CHASE_SPEED = 15.0f;        // Running speed during chase (10x patrol)
    static constexpr float ESCAPE_RADIUS = 15.0f;      // If player escapes beyond this, return to patrol
    static constexpr float TURN_SPEED = 5.0f;          // How fast monster rotates to face target
};
