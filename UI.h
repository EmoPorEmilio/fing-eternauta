#pragma once

#include "SDL.h"
#include "Settings.h"

struct UIState
{
    bool open = false;
    int selectedIndex = 0;
    int page = 0;
    int scrollIndex = 0; // first visible row index for the current tab
};

void UI_Initialize(SDL_Window *window);
void UI_Shutdown();

// Returns true if the event was consumed by the UI
bool UI_HandleEvent(const SDL_Event &e, UIState &state, AppSettings &settings, int windowWidth, int windowHeight, bool &needsRegenerate, bool &needsShaderReload);

void UI_BeginFrame();
void UI_Draw(const UIState &state, const AppSettings &settings, int windowWidth, int windowHeight);

// Status from simulation
void UI_SetGustActive(bool active);
void UI_SetDebugStats(int active, int bvhVisible, int drawn, int culledOff, int culledTiny, int budgetHit);
void UI_DrawCountersMini(int windowWidth, int windowHeight, int active, int bvhVisible, int drawn, int culledOff, int culledTiny, int budgetHit);

inline bool UI_IsOpen(const UIState &state) { return state.open; }
