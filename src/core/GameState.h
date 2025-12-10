#pragma once
#include "GameConfig.h"

// Runtime game state - mutable values that change during gameplay
// This separates config (constants) from state (variables)

struct GameState {
    // Visual effects toggles
    bool fogEnabled = false;
    bool snowEnabled = true;
    bool toonShadingEnabled = false;

    // Snow effect parameters
    float snowSpeed = GameConfig::SNOW_DEFAULT_SPEED;
    float snowAngle = GameConfig::SNOW_DEFAULT_ANGLE;
    float snowMotionBlur = GameConfig::SNOW_DEFAULT_BLUR;

    // Menu state
    int menuSelection = 0;       // Main menu: 0 = Play Game, 1 = God Mode
    int pauseMenuSelection = 0;  // Pause menu selection index

    // Intro text typewriter state
    int introCurrentLine = 0;
    size_t introCurrentChar = 0;
    float introTypewriterTimer = 0.0f;
    float introLinePauseTimer = 0.0f;
    bool introLineComplete = false;
    bool introAllComplete = false;

    // Motion blur state (for cinematic)
    bool motionBlurInitialized = false;
    int motionBlurPingPong = 0;

    // Building culling state
    int lastPlayerGridX = -9999;
    int lastPlayerGridZ = -9999;

    // LOD state
    bool fingUsingHighDetail = false;

    // Timing
    float gameTime = 0.0f;

    // Reset intro text state (for re-entering intro scene)
    void resetIntroText() {
        introCurrentLine = 0;
        introCurrentChar = 0;
        introTypewriterTimer = 0.0f;
        introLinePauseTimer = 0.0f;
        introLineComplete = false;
        introAllComplete = false;
    }

    // Reset motion blur state (for re-entering cinematic)
    void resetMotionBlur() {
        motionBlurInitialized = false;
        motionBlurPingPong = 0;
    }
};
