#include "UI.h"

#include <string>
#include <vector>
#include <algorithm>
#include <glad/glad.h>
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include "SDL_syswm.h"

// Very simple bitmap text rendering using OpenGL old-style GLUT-like bitmaps is not available.
// We'll draw a minimalistic immediate-mode style overlay using quads and simple bitmap text via wglUseFontBitmaps.
// To keep it simple and avoid new shader pipelines, we draw after the 3D scene with glDisable(GL_DEPTH_TEST)
// and use glBegin/glEnd for portability in this demo.

// Forward declarations for local helpers used before their definitions
static void setupOrtho(int width, int height);
static void restoreMatrices();
static void drawText(float x, float y, const std::string &text, float r, float g, float b, float a);

static void drawFilledRect(float x, float y, float w, float h, float r, float g, float b, float a)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static GLuint g_fontBase = 0;
static bool g_fontReady = false;
static bool g_gustActiveBadge = false;
static int g_statActive = 0;
static int g_statBVH = 0;
static int g_statDrawn = 0;
static int g_statOff = 0;
static int g_statTiny = 0;
static int g_statCap = 0;

void UI_SetGustActive(bool active) { g_gustActiveBadge = active; }
void UI_SetDebugStats(int active, int bvhVisible, int drawn, int culledOff, int culledTiny, int budgetHit)
{
    g_statActive = active;
    g_statBVH = bvhVisible;
    g_statDrawn = drawn;
    g_statOff = culledOff;
    g_statTiny = culledTiny;
    g_statCap = budgetHit;
}

void UI_DrawCountersMini(int windowWidth, int windowHeight, int active, int bvhVisible, int drawn, int culledOff, int culledTiny, int budgetHit)
{
    if (!g_fontReady)
        return;
    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    setupOrtho(windowWidth, windowHeight);

    // Cadence label (top-left)
    {
        float wL = 120.0f, hL = 28.0f, xL = 12.0f, yL = 12.0f;
        drawFilledRect(xL, yL, wL, hL, 0.06f, 0.08f, 0.12f, 0.85f);
        extern AppSettings gSettings; // declared in main.cpp
        const char *selText = (gSettings.cadenceSelection == CadenceSelection::One)     ? "Cycle: 1"
                              : (gSettings.cadenceSelection == CadenceSelection::Two)   ? "Cycle: 2"
                              : (gSettings.cadenceSelection == CadenceSelection::Three) ? "Cycle: 3"
                                                                                        : "Cycle: Auto";
        drawText(xL + 10.0f, yL + 18.0f, selText, 0.9f, 0.95f, 1.0f, 1.0f);
    }

    float w = 260.0f, h = 98.0f, x = windowWidth - w - 16.0f, y = 12.0f;
    drawFilledRect(x, y, w, h, 0.06f, 0.08f, 0.12f, 0.85f);
    char line[64];
    float ly = y + 22.0f;
    snprintf(line, sizeof(line), "Active: %d", active);
    drawText(x + 10.0f, ly, line, 0.9f, 0.95f, 1.0f, 1.0f);
    ly += 16.0f;
    snprintf(line, sizeof(line), "BVH: %d", bvhVisible);
    drawText(x + 10.0f, ly, line, 0.9f, 0.95f, 1.0f, 1.0f);
    ly += 16.0f;
    snprintf(line, sizeof(line), "Drawn: %d", drawn);
    drawText(x + 10.0f, ly, line, 0.9f, 0.95f, 1.0f, 1.0f);
    ly += 16.0f;
    snprintf(line, sizeof(line), "Off: %d  Tiny: %d", culledOff, culledTiny);
    drawText(x + 10.0f, ly, line, 0.9f, 0.95f, 1.0f, 1.0f);
    ly += 16.0f;
    snprintf(line, sizeof(line), "Cap: %d", budgetHit);
    drawText(x + 10.0f, ly, line, 0.9f, 0.95f, 1.0f, 1.0f);
    restoreMatrices();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(prevProgram);
}

static void drawText(float x, float y, const std::string &text, float r, float g, float b, float a)
{
    if (!g_fontReady || text.empty())
        return;
    glColor4f(r, g, b, a);
    glRasterPos2f(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(g_fontBase);
    glCallLists((GLsizei)text.size(), GL_UNSIGNED_BYTE, text.c_str());
    glPopAttrib();
}

static void setupOrtho(int width, int height)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

static void restoreMatrices()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void UI_Initialize(SDL_Window *window)
{
    if (g_fontReady)
        return;
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    if (!SDL_GetWindowWMInfo(window, &wminfo))
        return;
    HWND hwnd = wminfo.info.win.window;
    HDC hdc = GetDC(hwnd);
    if (!hdc)
        return;

    HFONT hFont = CreateFontA(
        -16, 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        FIXED_PITCH | FF_DONTCARE, "Consolas");
    if (!hFont)
    {
        ReleaseDC(hwnd, hdc);
        return;
    }
    HFONT hOld = (HFONT)SelectObject(hdc, hFont);

    if (g_fontBase == 0)
        g_fontBase = glGenLists(256);

    BOOL ok = wglUseFontBitmapsA(hdc, 0, 256, g_fontBase);
    SelectObject(hdc, hOld);
    DeleteObject(hFont);
    ReleaseDC(hwnd, hdc);
    if (ok)
        g_gustActiveBadge = false, g_fontReady = true;
}

void UI_Shutdown()
{
    if (g_fontBase)
    {
        glDeleteLists(g_fontBase, 256);
        g_fontBase = 0;
    }
    g_fontReady = false;
}

// Global setting index labels (0..19)
static std::string labelForIndex(int index)
{
    switch (index)
    {
    case 0:
        return "Pyramids";
    case 1:
        return "Shader";
    case 2:
        return "FOV";
    case 3:
        return "Near Plane";
    case 4:
        return "Far Plane";
    case 5:
        return "Frustum Culling";
    case 6:
        return "Rotation";
    case 7:
        return "Camera Speed";
    case 8:
        return "Mouse Sensitivity";
    case 9:
        return "Background R";
    case 10:
        return "Background G";
    case 11:
        return "Background B";
    case 12:
        return "VSync";
    case 13:
        return "Light Pos X";
    case 14:
        return "Light Pos Y";
    case 15:
        return "Light Pos Z";
    case 16:
        return "Ambient";
    case 17:
        return "Diffuse";
    case 18:
        return "Specular";
    case 19:
        return "Shininess";
    default:
        return "";
    }
}

struct TabDef
{
    const char *name;
    std::vector<int> indices;
};

static std::vector<TabDef> getTabs()
{
    std::vector<TabDef> tabs;
    tabs.push_back({"Rendering", {1, 2, 3, 4, 12, 9, 10, 11}});
    tabs.push_back({"Camera", {7, 8}});
    tabs.push_back({"Scene", {0, 5, 6, 13, 14, 15}});
    tabs.push_back({"Lighting", {16, 17, 18, 19}});
    tabs.push_back({"Cadence", {}});
    tabs.push_back({"Debug", {}});
    return tabs;
}

static void applyAdjustmentByIndex(int index, int dir, AppSettings &settings, bool &needsRegenerate, bool &needsShaderReload)
{
    if (index == 0)
    {
        settings.targetPyramidCount = std::max(0, settings.targetPyramidCount + dir * 100);
        needsRegenerate = true;
        return;
    }
    if (index == 1)
    {
        int st = (int)settings.shaderType;
        st = (st + (dir > 0 ? 1 : -1) + 5) % 5; // cycle Phong, Basic, SnowGlow, FrostCrystal, Mix
        settings.shaderType = (ShaderType)st;
        needsShaderReload = true;
        return;
    }
    if (index == 2)
    {
        settings.fovDegrees = std::max(20.0f, std::min(120.0f, settings.fovDegrees + dir * 5.0f));
        return;
    }
    if (index == 3)
    {
        settings.nearPlane = std::max(0.01f, std::min(settings.farPlane - 0.1f, settings.nearPlane + dir * 0.05f));
        return;
    }
    if (index == 4)
    {
        settings.farPlane = std::max(settings.nearPlane + 1.0f, settings.farPlane + dir * 5.0f);
        return;
    }
    if (index == 5)
    {
        settings.frustumCullingEnabled = !settings.frustumCullingEnabled;
        return;
    }
    if (index == 6)
    {
        settings.enableRotation = !settings.enableRotation;
        return;
    }
    if (index == 7)
    {
        settings.cameraSpeed = std::max(0.1f, std::min(20.0f, settings.cameraSpeed + dir * 0.2f));
        return;
    }
    if (index == 8)
    {
        settings.mouseSensitivity = std::max(0.01f, std::min(1.0f, settings.mouseSensitivity + dir * 0.01f));
        return;
    }
    if (index == 9)
    {
        settings.bgR = std::max(0.0f, std::min(1.0f, settings.bgR + dir * 0.05f));
        return;
    }
    if (index == 10)
    {
        settings.bgG = std::max(0.0f, std::min(1.0f, settings.bgG + dir * 0.05f));
        return;
    }
    if (index == 11)
    {
        settings.bgB = std::max(0.0f, std::min(1.0f, settings.bgB + dir * 0.05f));
        return;
    }
    if (index == 12)
    {
        settings.vsyncEnabled = !settings.vsyncEnabled;
        return;
    }
    if (index == 13)
    {
        settings.lightPosX = settings.lightPosX + dir * 0.5f;
        return;
    }
    if (index == 14)
    {
        settings.lightPosY = settings.lightPosY + dir * 0.5f;
        return;
    }
    if (index == 15)
    {
        settings.lightPosZ = settings.lightPosZ + dir * 0.5f;
        return;
    }
    if (index == 16)
    {
        settings.ambientStrength = std::max(0.0f, std::min(1.0f, settings.ambientStrength + dir * 0.05f));
        return;
    }
    if (index == 17)
    {
        settings.diffuseStrength = std::max(0.0f, std::min(1.0f, settings.diffuseStrength + dir * 0.05f));
        return;
    }
    if (index == 18)
    {
        settings.specularStrength = std::max(0.0f, std::min(1.0f, settings.specularStrength + dir * 0.05f));
        return;
    }
    if (index == 19)
    {
        settings.shininess = std::max(1.0f, std::min(128.0f, settings.shininess + dir * 4.0f));
        return;
    }
}

bool UI_HandleEvent(const SDL_Event &e, UIState &state, AppSettings &settings, int windowWidth, int windowHeight, bool &needsRegenerate, bool &needsShaderReload)
{
    needsRegenerate = false;
    needsShaderReload = false;
    if (e.type == SDL_KEYDOWN)
    {
        if (e.key.keysym.sym == SDLK_RETURN)
        {
            state.open = !state.open;
            return true;
        }
        if (!state.open)
            return false;
        if (e.key.keysym.sym == SDLK_UP)
        {
            state.selectedIndex = std::max(0, state.selectedIndex - 1);
            if (state.selectedIndex < state.scrollIndex)
                state.scrollIndex = state.selectedIndex;
            return true;
        }
        if (e.key.keysym.sym == SDLK_DOWN)
        {
            state.selectedIndex = state.selectedIndex + 1;
            return true;
        }
        if (e.key.keysym.sym == SDLK_PAGEUP)
        {
            state.selectedIndex = std::max(0, state.selectedIndex - 5);
            state.scrollIndex = std::max(0, state.scrollIndex - 5);
            return true;
        }
        if (e.key.keysym.sym == SDLK_PAGEDOWN)
        {
            state.selectedIndex = state.selectedIndex + 5;
            return true;
        }
        if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_RIGHT)
        {
            int dir = (e.key.keysym.sym == SDLK_RIGHT) ? 1 : -1;
            auto tabs = getTabs();
            int tab = std::max(0, std::min(state.page, (int)tabs.size() - 1));
            if (tabs[tab].name == std::string("Cadence"))
            {
                int local = state.selectedIndex;
                // Cadence rows: 0 Mode, 1 Cycle, 2..10 Presets, 11 Gust toggle, 12 Gust interval, 13 Gust duration, 14 Gust fall mul, 15 Gust rot mul
                if (local == 0)
                {
                    int sel = (int)settings.cadenceSelection;
                    sel = (sel + (dir > 0 ? 1 : -1) + 4) % 4;
                    settings.cadenceSelection = (CadenceSelection)sel;
                    return true;
                }
                else if (local == 1)
                {
                    settings.cadenceCycleSeconds = std::max(5.0f, std::min(300.0f, settings.cadenceCycleSeconds + dir * 5.0f));
                    return true;
                }
                else if (local >= 2 && local <= 10)
                {
                    int preset = (local - 2) / 3;
                    int field = (local - 2) % 3;
                    if (field == 0)
                    {
                        settings.cadence[preset].pyramids = std::max(0, settings.cadence[preset].pyramids + dir * 100);
                        needsRegenerate = true;
                    }
                    if (field == 1)
                    {
                        settings.cadence[preset].rotationScale = std::max(0.0f, std::min(5.0f, settings.cadence[preset].rotationScale + dir * 0.1f));
                    }
                    if (field == 2)
                    {
                        settings.cadence[preset].fallSpeed = std::max(0.0f, std::min(5.0f, settings.cadence[preset].fallSpeed + dir * 0.1f));
                    }
                    return true;
                }
                else if (local == 11)
                {
                    settings.gustsEnabled = !settings.gustsEnabled;
                    return true;
                }
                else if (local == 12)
                {
                    settings.gustIntervalSeconds = std::max(2.0f, std::min(60.0f, settings.gustIntervalSeconds + dir * 1.0f));
                    return true;
                }
                else if (local == 13)
                {
                    settings.gustDurationSeconds = std::max(0.5f, std::min(10.0f, settings.gustDurationSeconds + dir * 0.5f));
                    return true;
                }
                else if (local == 14)
                {
                    settings.gustFallMultiplier = std::max(1.0f, std::min(5.0f, settings.gustFallMultiplier + dir * 0.1f));
                    return true;
                }
                else if (local == 15)
                {
                    settings.gustRotationMultiplier = std::max(1.0f, std::min(5.0f, settings.gustRotationMultiplier + dir * 0.1f));
                    return true;
                }
                return true;
            }
            const std::vector<int> &indices = tabs[tab].indices;
            int count = (int)indices.size();
            if (state.selectedIndex >= 0 && state.selectedIndex < count)
            {
                int globalIndex = indices[state.selectedIndex];
                applyAdjustmentByIndex(globalIndex, dir, settings, needsRegenerate, needsShaderReload);
                return true;
            }
        }
    }
    else if (e.type == SDL_MOUSEWHEEL)
    {
        if (!state.open)
            return false;
        int delta = (e.wheel.y > 0) ? -1 : (e.wheel.y < 0 ? 1 : 0);
        state.scrollIndex = std::max(0, state.scrollIndex + delta);
        if (state.selectedIndex < state.scrollIndex)
            state.selectedIndex = state.scrollIndex;
        return true;
    }
    else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        if (!state.open)
            return false;
        const float panelW = (float)std::min(480, windowWidth - 40);
        const float panelH = (float)std::min(460, windowHeight - 40);
        const float x = 20.0f;
        const float y = 20.0f;
        float mx = (float)e.button.x;
        float my = (float)e.button.y;
        auto inRect = [&](float rx, float ry, float rw, float rh)
        {
            return mx >= rx && mx <= rx + rw && my >= ry && my <= ry + rh;
        };
        float tabBarY = y + 44.0f;
        float tabBarH = 24.0f;
        auto tabs = getTabs();
        float tabX = x + 10.0f;
        float tabW = 110.0f;
        for (int ti = 0; ti < (int)tabs.size(); ++ti)
        {
            if (inRect(tabX, tabBarY, tabW, tabBarH))
            {
                state.page = ti;
                state.selectedIndex = 0;
                state.scrollIndex = 0;
                return true;
            }
            tabX += tabW + 8.0f;
        }
        float itemH = 28.0f;
        float startY = y + 76.0f;
        int tab = std::max(0, std::min(state.page, (int)tabs.size() - 1));
        if (tabs[tab].name == std::string("Cadence"))
        {
            int total = 16; // includes gust controls
            int visibleRows = (int)((panelH - 76.0f - 34.0f - 10.0f) / (itemH + 6.0f));
            int startRow = state.scrollIndex;
            int endRow = std::min(startRow + visibleRows, total);
            for (int local = startRow; local < endRow; ++local)
            {
                float iy = startY + (local - startRow) * (itemH + 6.0f);
                if (my >= iy && my <= iy + itemH && mx >= x + 10.0f && mx <= x + panelW - 10.0f)
                {
                    state.selectedIndex = local;
                    float rightX = x + panelW - 180.0f;
                    float boxW = 120.0f;
                    if (mx >= rightX && mx <= rightX + boxW)
                    {
                        if (local == 0)
                        {
                            int sel = (int)settings.cadenceSelection;
                            sel = (sel + 1) % 4;
                            settings.cadenceSelection = (CadenceSelection)sel;
                            return true;
                        }
                        if (local == 1)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.cadenceCycleSeconds = std::max(5.0f, std::min(300.0f, settings.cadenceCycleSeconds + dir * 5.0f));
                            return true;
                        }
                        if (local >= 2 && local <= 10)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            int preset = (local - 2) / 3;
                            int field = (local - 2) % 3;
                            if (field == 0)
                            {
                                settings.cadence[preset].pyramids = std::max(0, settings.cadence[preset].pyramids + dir * 100);
                                needsRegenerate = true;
                            }
                            if (field == 1)
                            {
                                settings.cadence[preset].rotationScale = std::max(0.0f, std::min(5.0f, settings.cadence[preset].rotationScale + dir * 0.1f));
                            }
                            if (field == 2)
                            {
                                settings.cadence[preset].fallSpeed = std::max(0.0f, std::min(5.0f, settings.cadence[preset].fallSpeed + dir * 0.1f));
                            }
                            return true;
                        }
                        if (local == 11)
                        {
                            settings.gustsEnabled = !settings.gustsEnabled;
                            return true;
                        }
                        if (local == 12)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.gustIntervalSeconds = std::max(2.0f, std::min(60.0f, settings.gustIntervalSeconds + dir * 1.0f));
                            return true;
                        }
                        if (local == 13)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.gustDurationSeconds = std::max(0.5f, std::min(10.0f, settings.gustDurationSeconds + dir * 0.5f));
                            return true;
                        }
                        if (local == 14)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.gustFallMultiplier = std::max(1.0f, std::min(5.0f, settings.gustFallMultiplier + dir * 0.1f));
                            return true;
                        }
                        if (local == 15)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.gustRotationMultiplier = std::max(1.0f, std::min(5.0f, settings.gustRotationMultiplier + dir * 0.1f));
                            return true;
                        }
                    }
                    return true;
                }
            }
        }
        // Rendering and others handled below
        const std::vector<int> &indices = tabs[tab].indices;
        int baseTotal = (int)indices.size();
        int extra = (tab == 0 && settings.shaderType == ShaderType::SnowGlow) ? 10 : 0;
        int materialExtra = (tab == 0 && settings.shaderType == ShaderType::SnowGlow) ? 9 : 0;
        int total = baseTotal + extra + materialExtra;
        int visibleRows = (int)((panelH - 76.0f - 34.0f - 10.0f) / (itemH + 6.0f));
        int startRow = state.scrollIndex;
        int endRow = std::min(startRow + visibleRows, total);
        for (int local = startRow; local < endRow; ++local)
        {
            float iy = startY + (local - startRow) * (itemH + 6.0f);
            if (my >= iy && my <= iy + itemH && mx >= x + 10.0f && mx <= x + panelW - 10.0f)
            {
                state.selectedIndex = local;
                float rightX = x + panelW - 180.0f;
                if (mx >= rightX && mx <= rightX + 120.0f)
                {
                    int dir = (mx < rightX + 60.0f) ? -1 : +1;
                    if (local < baseTotal)
                    {
                        int globalIndex = indices[local];
                        applyAdjustmentByIndex(globalIndex, dir, settings, needsRegenerate, needsShaderReload);
                    }
                    else if (local < baseTotal + extra)
                    {
                        // SnowGlow extra controls (glow/sparkle/noise/tint/fog + rim/power/exposure + mix)
                        int eidx = local - baseTotal;
                        if (eidx == 0)
                            settings.snowGlowIntensity = std::max(0.0f, settings.snowGlowIntensity + dir * 0.1f);
                        if (eidx == 1)
                            settings.snowSparkleIntensity = std::max(0.0f, settings.snowSparkleIntensity + dir * 0.1f);
                        if (eidx == 2)
                            settings.snowSparkleThreshold = std::max(0.0f, std::min(1.0f, settings.snowSparkleThreshold + dir * 0.02f));
                        if (eidx == 3)
                            settings.snowNoiseScale = std::max(0.01f, std::min(5.0f, settings.snowNoiseScale + dir * 0.05f));
                        if (eidx == 4)
                            settings.snowTintStrength = std::max(0.0f, std::min(1.0f, settings.snowTintStrength + dir * 0.02f));
                        if (eidx == 5)
                            settings.snowFogStrength = std::max(0.0f, std::min(1.0f, settings.snowFogStrength + dir * 0.02f));
                        if (eidx == 6)
                            settings.snowRimStrength = std::max(0.0f, std::min(2.0f, settings.snowRimStrength + dir * 0.05f));
                        if (eidx == 7)
                            settings.snowRimPower = std::max(0.5f, std::min(6.0f, settings.snowRimPower + dir * 0.1f));
                        if (eidx == 8)
                            settings.snowExposure = std::max(0.2f, std::min(3.0f, settings.snowExposure + dir * 0.05f));
                        if (eidx == 9)
                            settings.snowMixAmount = std::max(0.0f, std::min(1.0f, settings.snowMixAmount + dir * 0.05f));
                        // New depth/fog aesthetics
                        if (eidx == 10)
                            settings.depthDesatStrength = std::max(0.0f, std::min(1.0f, settings.depthDesatStrength + dir * 0.05f));
                        if (eidx == 11)
                            settings.depthBlueStrength = std::max(0.0f, std::min(1.0f, settings.depthBlueStrength + dir * 0.05f));
                        if (eidx == 12)
                            settings.fogHeightStrength = std::max(0.0f, std::min(1.0f, settings.fogHeightStrength + dir * 0.05f));
                    }
                    else
                    {
                        // Snow material parameters
                        int midx = local - baseTotal - extra;
                        if (midx == 0)
                            settings.snowRoughness = std::max(0.0f, std::min(1.0f, settings.snowRoughness + dir * 0.05f));
                        if (midx == 1)
                            settings.snowMetallic = std::max(0.0f, std::min(1.0f, settings.snowMetallic + dir * 0.05f));
                        if (midx == 2)
                            settings.snowSSS = std::max(0.0f, std::min(1.0f, settings.snowSSS + dir * 0.05f));
                        if (midx == 3)
                            settings.snowAnisotropy = std::max(0.0f, std::min(1.0f, settings.snowAnisotropy + dir * 0.05f));
                        if (midx == 4)
                            settings.snowBaseAlpha = std::max(0.0f, std::min(1.0f, settings.snowBaseAlpha + dir * 0.05f));
                        if (midx == 5)
                            settings.snowEdgeFade = std::max(0.0f, std::min(1.0f, settings.snowEdgeFade + dir * 0.05f));
                        if (midx == 6)
                            settings.snowNormalAmplitude = std::max(0.0f, std::min(1.0f, settings.snowNormalAmplitude + dir * 0.05f));
                        if (midx == 7)
                            settings.snowCrackScale = std::max(0.0f, std::min(1.0f, settings.snowCrackScale + dir * 0.05f));
                        if (midx == 8)
                            settings.snowCrackIntensity = std::max(0.0f, std::min(1.0f, settings.snowCrackIntensity + dir * 0.05f));
                    }
                }
                return true;
            }
        }
        if (tabs[tab].name == std::string("Debug"))
        {
            // Debug controls: overlay, speed, size, distance culling, screen culling, uniform batching, min/max size, surfaces scale/toggles
            int total = 13;
            int visibleRows = (int)((panelH - 76.0f - 34.0f - 10.0f) / (itemH + 6.0f));
            int startRow = state.scrollIndex;
            int endRow = std::min(startRow + visibleRows, total);
            for (int local = startRow; local < endRow; ++local)
            {
                float iy = startY + (local - startRow) * (itemH + 6.0f);
                if (my >= iy && my <= iy + itemH && mx >= x + 10.0f && mx <= x + panelW - 10.0f)
                {
                    state.selectedIndex = local;
                    float rightX = x + panelW - 180.0f;
                    float boxW = 120.0f;
                    if (mx >= rightX && mx <= rightX + boxW)
                    {
                        if (local == 0)
                        {
                            settings.debugOverlayEnabled = !settings.debugOverlayEnabled;
                            return true;
                        }
                        if (local == 1)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.impostorSpeedMultiplier = std::max(1.0f, std::min(100.0f, settings.impostorSpeedMultiplier + dir * 1.0f));
                            return true;
                        }
                        if (local == 2)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.impostorSizeMultiplier = std::max(0.2f, std::min(4.0f, settings.impostorSizeMultiplier + dir * 0.1f));
                            return true;
                        }
                        if (local == 3)
                        {
                            settings.enableDistanceCulling = !settings.enableDistanceCulling;
                            return true;
                        }
                        if (local == 4)
                        {
                            settings.enableScreenSpaceCulling = !settings.enableScreenSpaceCulling;
                            return true;
                        }
                        if (local == 5)
                        {
                            settings.enableUniformBatching = !settings.enableUniformBatching;
                            return true;
                        }
                        if (local == 6)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.impostorMinWorldSize = std::max(0.01f, std::min(settings.impostorMaxWorldSize, settings.impostorMinWorldSize + dir * 0.05f));
                            return true;
                        }
                        if (local == 7)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.impostorMaxWorldSize = std::max(settings.impostorMinWorldSize, std::min(10.0f, settings.impostorMaxWorldSize + dir * 0.1f));
                            return true;
                        }
                        if (local == 8)
                        {
                            int dir = (mx < rightX + 60.0f) ? -1 : +1;
                            settings.surfaceScale = std::max(0.25f, std::min(4.0f, settings.surfaceScale + dir * 0.1f));
                            return true;
                        }
                        if (local == 9)
                        {
                            settings.sidePlatformEnabled = !settings.sidePlatformEnabled;
                            return true;
                        }
                        if (local == 10)
                        {
                            settings.shelfEnabled = !settings.shelfEnabled;
                            return true;
                        }
                        if (local == 11)
                        {
                            settings.crateEnabled = !settings.crateEnabled;
                            return true;
                        }
                        if (local == 12)
                        {
                            settings.columnEnabled = !settings.columnEnabled;
                            return true;
                        }
                    }
                    return true;
                }
            }
        }

        if (inRect(x, y, panelW, panelH))
            return true;
    }
    // Clamp selection/scroll
    auto tabs = getTabs();
    int tab = std::max(0, std::min(state.page, (int)tabs.size() - 1));
    int baseTotal = (tabs[tab].name == std::string("Cadence")) ? 16 : (tabs[tab].name == std::string("Debug") ? 8 : (int)tabs[tab].indices.size());
    if (tab == 0 && tabs[tab].name == std::string("Rendering"))
    {
        if (baseTotal == (int)tabs[tab].indices.size())
        {
            // adjust for snowglow extras
            if (true)
            { /* placeholder */
            }
        }
    }
    int total = baseTotal;
    if (tab == 0 && settings.shaderType == ShaderType::SnowGlow)
        total = (int)tabs[tab].indices.size() + 10;
    if (total <= 0)
    {
        state.selectedIndex = 0;
        state.scrollIndex = 0;
        return false;
    }
    if (state.selectedIndex < 0)
        state.selectedIndex = 0;
    if (state.selectedIndex >= total)
        state.selectedIndex = total - 1;
    const float panelHc = (float)std::min(460, windowHeight - 40);
    float itemHc = 28.0f;
    int visibleRows = (int)((panelHc - 76.0f - 34.0f - 10.0f) / (itemHc + 6.0f));
    if (visibleRows < 1)
        visibleRows = 1;
    if (state.scrollIndex > state.selectedIndex)
        state.scrollIndex = state.selectedIndex;
    if (state.selectedIndex >= state.scrollIndex + visibleRows)
        state.scrollIndex = state.selectedIndex - visibleRows + 1;
    if (state.scrollIndex < 0)
        state.scrollIndex = 0;
    if (state.scrollIndex > std::max(0, total - visibleRows))
        state.scrollIndex = std::max(0, total - visibleRows);
    return false;
}

void UI_BeginFrame()
{
}

// Minimal menu with tabs, scrolling, and labels/switches
void UI_Draw(const UIState &state, const AppSettings &settings, int windowWidth, int windowHeight)
{
    if (!state.open)
        return;

    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
    glUseProgram(0);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    setupOrtho(windowWidth, windowHeight);

    const float panelW = (float)std::min(480, windowWidth - 40);
    const float panelH = (float)std::min(460, windowHeight - 40);
    const float x = 20.0f;
    const float y = 20.0f;

    // Panel background
    drawFilledRect(x, y, panelW, panelH, 0.06f, 0.08f, 0.12f, 0.92f);

    // Header bar
    drawFilledRect(x, y, panelW, 40.0f, 0.15f, 0.25f, 0.45f, 0.95f);
    drawText(x + 14.0f, y + 26.0f, "Settings", 0.95f, 0.98f, 1.0f, 1.0f);
    if (g_gustActiveBadge)
    {
        float bx = x + panelW - 110.0f, by = y + 8.0f;
        drawFilledRect(bx, by, 100.0f, 24.0f, 0.8f, 0.3f, 0.2f, 0.9f);
        drawText(bx + 10.0f, by + 18.0f, "GUST ACTIVE", 1.0f, 0.95f, 0.9f, 1.0f);
    }

    // Tab bar
    float tabBarY = y + 44.0f;
    float tabBarH = 24.0f;
    drawFilledRect(x, tabBarY, panelW, tabBarH, 0.1f, 0.14f, 0.22f, 0.95f);
    auto tabs = getTabs();
    float tabX = x + 10.0f;
    float tabW = 110.0f;
    int tab = std::max(0, std::min(state.page, (int)tabs.size() - 1));
    for (int ti = 0; ti < (int)tabs.size(); ++ti)
    {
        bool selected = (ti == tab);
        drawFilledRect(tabX, tabBarY + 2.0f, tabW, tabBarH - 4.0f, selected ? 0.2f : 0.18f, selected ? 0.4f : 0.22f, selected ? 0.8f : 0.32f, 0.85f);
        drawText(tabX + 10.0f, tabBarY + 18.0f, tabs[ti].name, 0.9f, 0.95f, 1.0f, 1.0f);
        tabX += tabW + 8.0f;
    }

    // Scrollable list
    float itemH = 28.0f;
    float startY = y + 76.0f;
    int visibleRows = (int)((panelH - 76.0f - 34.0f - 10.0f) / (itemH + 6.0f));
    if (visibleRows < 1)
        visibleRows = 1;

    if (tabs[tab].name == std::string("Cadence"))
    {
        int total = 16; // includes gust controls
        int startRow = state.scrollIndex;
        int endRow = std::min(startRow + visibleRows, total);
        float rightX = x + panelW - 180.0f;
        for (int local = startRow; local < endRow; ++local)
        {
            float iy = startY + (local - startRow) * (itemH + 6.0f);
            if (local == state.selectedIndex)
                drawFilledRect(x + 10.0f, iy, panelW - 20.0f, itemH, 0.2f, 0.4f, 0.8f, 0.65f);
            else
                drawFilledRect(x + 10.0f, iy, panelW - 20.0f, itemH, 0.2f, 0.2f, 0.25f, 0.55f);

            // Labels and values
            int idx = local;
            if (idx == 0)
            {
                drawText(x + 20.0f, iy + 18.0f, "Cadence Mode", 0.9f, 0.95f, 1.0f, 1.0f);
                const char *selText = (settings.cadenceSelection == CadenceSelection::One)     ? "1"
                                      : (settings.cadenceSelection == CadenceSelection::Two)   ? "2"
                                      : (settings.cadenceSelection == CadenceSelection::Three) ? "3"
                                                                                               : "Cycle";
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                drawText(rightX + 10.0f, iy + 18.0f, selText, 0.9f, 0.95f, 1.0f, 1.0f);
            }
            else if (idx == 1)
            {
                drawText(x + 20.0f, iy + 18.0f, "Cycle Seconds", 0.9f, 0.95f, 1.0f, 1.0f);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.cadenceCycleSeconds), 0.9f, 0.95f, 1.0f, 1.0f);
            }
            else if (idx >= 2 && idx <= 10)
            {
                int preset = (idx - 2) / 3;
                int field = (idx - 2) % 3;
                char title[32];
                if (field == 0)
                {
                    snprintf(title, sizeof(title), "Preset %d Pyramids", preset + 1);
                    drawText(x + 20.0f, iy + 18.0f, title, 0.9f, 0.95f, 1.0f, 1.0f);
                    drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                    drawText(rightX + 10.0f, iy + 18.0f, std::to_string(settings.cadence[preset].pyramids), 0.9f, 0.95f, 1.0f, 1.0f);
                }
                else if (field == 1)
                {
                    snprintf(title, sizeof(title), "Preset %d Rotation", preset + 1);
                    drawText(x + 20.0f, iy + 18.0f, title, 0.9f, 0.95f, 1.0f, 1.0f);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f", settings.cadence[preset].rotationScale);
                    drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                    drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                }
                else
                {
                    snprintf(title, sizeof(title), "Preset %d Fall Speed", preset + 1);
                    drawText(x + 20.0f, iy + 18.0f, title, 0.9f, 0.95f, 1.0f, 1.0f);
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f", settings.cadence[preset].fallSpeed);
                    drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                    drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                }
            }
            else if (idx == 11)
            {
                drawText(x + 20.0f, iy + 18.0f, "Gusts Enabled", 0.9f, 0.95f, 1.0f, 1.0f);
                drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.gustsEnabled ? 0.15f : 0.35f, settings.gustsEnabled ? 0.5f : 0.15f, settings.gustsEnabled ? 0.2f : 0.15f, 0.9f);
                drawText(rightX + 18.0f, iy + 18.0f, settings.gustsEnabled ? "ON" : "OFF", settings.gustsEnabled ? 0.8f : 1.0f, settings.gustsEnabled ? 1.0f : 0.8f, 0.85f, 1.0f);
            }
            else if (idx == 12)
            {
                drawText(x + 20.0f, iy + 18.0f, "Gust Interval s", 0.9f, 0.95f, 1.0f, 1.0f);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.gustIntervalSeconds), 0.9f, 0.95f, 1.0f, 1.0f);
            }
            else if (idx == 13)
            {
                drawText(x + 20.0f, iy + 18.0f, "Gust Duration s", 0.9f, 0.95f, 1.0f, 1.0f);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.gustDurationSeconds), 0.9f, 0.95f, 1.0f, 1.0f);
            }
            else if (idx == 14)
            {
                drawText(x + 20.0f, iy + 18.0f, "Gust Fall x", 0.9f, 0.95f, 1.0f, 1.0f);
                char buf[32];
                snprintf(buf, sizeof(buf), "%.2f", settings.gustFallMultiplier);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
            }
            else if (idx == 15)
            {
                drawText(x + 20.0f, iy + 18.0f, "Gust Rotation x", 0.9f, 0.95f, 1.0f, 1.0f);
                char buf[32];
                snprintf(buf, sizeof(buf), "%.2f", settings.gustRotationMultiplier);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
            }
        }
        // Scrollbar
        int totalRows = total;
        float trackX = x + panelW - 10.0f;
        float trackY = startY;
        float trackH = panelH - 76.0f - 34.0f - 10.0f;
        drawFilledRect(trackX, trackY, 4.0f, trackH, 0.15f, 0.2f, 0.3f, 0.6f);
        float thumbH = std::max(20.0f, trackH * (visibleRows / (float)std::max(1, totalRows)));
        float thumbY = trackY + (trackH - thumbH) * (state.scrollIndex / (float)std::max(1, totalRows - visibleRows));
        drawFilledRect(trackX, thumbY, 4.0f, thumbH, 0.35f, 0.55f, 0.9f, 0.9f);
    }
    else
    {
        const std::vector<int> &indices = tabs[tab].indices;
        int baseTotal = (int)indices.size();
        int extra = (settings.shaderType == ShaderType::SnowGlow) ? 6 : 0;
        int materialExtra = (settings.shaderType == ShaderType::SnowGlow) ? 9 : 0;
        int total = (tabs[tab].name == std::string("Debug")) ? 6 : baseTotal + extra + materialExtra;
        int startRow = state.scrollIndex;
        int endRow = std::min(startRow + visibleRows, total);
        float rightX = x + panelW - 180.0f;
        for (int local = startRow; local < endRow; ++local)
        {
            float iy = startY + (local - startRow) * (itemH + 6.0f);
            if (local == state.selectedIndex)
                drawFilledRect(x + 10.0f, iy, panelW - 20.0f, itemH, 0.2f, 0.4f, 0.8f, 0.65f);
            else
                drawFilledRect(x + 10.0f, iy, panelW - 20.0f, itemH, 0.2f, 0.2f, 0.25f, 0.55f);

            if (tabs[tab].name == std::string("Debug"))
            {
                const char *labelsAll[13] = {
                    "Overlay", "Speed x", "Size x",
                    "Dist Cull", "Screen Cull", "Batch Uniforms",
                    "Min Size", "Max Size",
                    "Surface Scale",
                    "Side Platform", "Shelf", "Crate", "Column"};
                drawText(x + 20.0f, iy + 18.0f, labelsAll[local], 0.9f, 0.95f, 1.0f, 1.0f);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);

                if (local == 0)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.debugOverlayEnabled ? 0.15f : 0.35f, settings.debugOverlayEnabled ? 0.5f : 0.15f, settings.debugOverlayEnabled ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.debugOverlayEnabled ? "ON" : "OFF", settings.debugOverlayEnabled ? 0.8f : 1.0f, settings.debugOverlayEnabled ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                else if (local == 1)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.1f", settings.impostorSpeedMultiplier);
                    drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                }
                else if (local == 2)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.1f", settings.impostorSizeMultiplier);
                    drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                }
                else if (local == 3)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.enableDistanceCulling ? 0.15f : 0.35f, settings.enableDistanceCulling ? 0.5f : 0.15f, settings.enableDistanceCulling ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.enableDistanceCulling ? "ON" : "OFF", settings.enableDistanceCulling ? 0.8f : 1.0f, settings.enableDistanceCulling ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                else if (local == 4)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.enableScreenSpaceCulling ? 0.15f : 0.35f, settings.enableScreenSpaceCulling ? 0.5f : 0.15f, settings.enableScreenSpaceCulling ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.enableScreenSpaceCulling ? "ON" : "OFF", settings.enableScreenSpaceCulling ? 0.8f : 1.0f, settings.enableScreenSpaceCulling ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                else if (local == 5)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.enableUniformBatching ? 0.15f : 0.35f, settings.enableUniformBatching ? 0.5f : 0.15f, settings.enableUniformBatching ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.enableUniformBatching ? "ON" : "OFF", settings.enableUniformBatching ? 0.8f : 1.0f, settings.enableUniformBatching ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                else if (local == 6)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f", settings.impostorMinWorldSize);
                    drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                }
                else if (local == 7)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f", settings.impostorMaxWorldSize);
                    drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                }
                else if (local == 8)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2f", settings.surfaceScale);
                    drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                }
                else if (local == 9)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.sidePlatformEnabled ? 0.15f : 0.35f, settings.sidePlatformEnabled ? 0.5f : 0.15f, settings.sidePlatformEnabled ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.sidePlatformEnabled ? "ON" : "OFF", settings.sidePlatformEnabled ? 0.8f : 1.0f, settings.sidePlatformEnabled ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                else if (local == 10)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.shelfEnabled ? 0.15f : 0.35f, settings.shelfEnabled ? 0.5f : 0.15f, settings.shelfEnabled ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.shelfEnabled ? "ON" : "OFF", settings.shelfEnabled ? 0.8f : 1.0f, settings.shelfEnabled ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                else if (local == 11)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.crateEnabled ? 0.15f : 0.35f, settings.crateEnabled ? 0.5f : 0.15f, settings.crateEnabled ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.crateEnabled ? "ON" : "OFF", settings.crateEnabled ? 0.8f : 1.0f, settings.crateEnabled ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                else if (local == 12)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, settings.columnEnabled ? 0.15f : 0.35f, settings.columnEnabled ? 0.5f : 0.15f, settings.columnEnabled ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, settings.columnEnabled ? "ON" : "OFF", settings.columnEnabled ? 0.8f : 1.0f, settings.columnEnabled ? 1.0f : 0.8f, 0.85f, 1.0f);
                }
                continue;
            }

            if (local < baseTotal)
            {
                int globalIndex = indices[local];
                drawText(x + 20.0f, iy + 18.0f, labelForIndex(globalIndex), 0.9f, 0.95f, 1.0f, 1.0f);

                if (globalIndex == 5 || globalIndex == 6 || globalIndex == 12)
                {
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                    bool on = (globalIndex == 5 ? settings.frustumCullingEnabled : (globalIndex == 6 ? settings.enableRotation : settings.vsyncEnabled));
                    drawFilledRect(rightX, iy + 6.0f, 80.0f, itemH - 12.0f, on ? 0.15f : 0.35f, on ? 0.5f : 0.15f, on ? 0.2f : 0.15f, 0.9f);
                    drawText(rightX + 18.0f, iy + 18.0f, on ? "ON" : "OFF", on ? 0.8f : 1.0f, on ? 1.0f : 0.8f, on ? 0.85f : 0.85f, 1.0f);
                }
                else
                {
                    drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                    switch (globalIndex)
                    {
                    case 0:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string(settings.targetPyramidCount), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 1:
                    {
                        const char *sname =
                            settings.shaderType == ShaderType::Phong ? "Phong" : settings.shaderType == ShaderType::Basic      ? "Basic"
                                                                             : settings.shaderType == ShaderType::SnowGlow     ? "SnowGlow"
                                                                             : settings.shaderType == ShaderType::FrostCrystal ? "FrostCrystal"
                                                                                                                               : "Mix";
                        drawText(rightX + 10.0f, iy + 18.0f, sname, 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    }
                    break;
                    case 2:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.fovDegrees) + " deg", 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 3:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string(settings.nearPlane), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 4:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.farPlane), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 7:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string(settings.cameraSpeed), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 8:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string(settings.mouseSensitivity), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 9:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)(settings.bgR * 255.0f)), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 10:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)(settings.bgG * 255.0f)), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 11:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)(settings.bgB * 255.0f)), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 13:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.lightPosX), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 14:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.lightPosY), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 15:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.lightPosZ), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    case 16:
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2f", settings.ambientStrength);
                        drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    }
                    case 17:
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2f", settings.diffuseStrength);
                        drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    }
                    case 18:
                    {
                        char buf[32];
                        snprintf(buf, sizeof(buf), "%.2f", settings.specularStrength);
                        drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    }
                    case 19:
                        drawText(rightX + 10.0f, iy + 18.0f, std::to_string((int)settings.shininess), 0.9f, 0.95f, 1.0f, 1.0f);
                        break;
                    default:
                        break;
                    }
                }
            }
            else if (local < baseTotal + extra)
            {
                // SnowGlow extra controls (glow/sparkle/noise/tint/fog + rim/exposure + mix)
                int eidx = local - baseTotal;
                const char *label =
                    (eidx == 0) ? "Glow Intensity" : (eidx == 1) ? "Sparkle Intensity"
                                                 : (eidx == 2)   ? "Sparkle Threshold"
                                                 : (eidx == 3)   ? "Noise Scale"
                                                 : (eidx == 4)   ? "Tint Strength"
                                                 : (eidx == 5)   ? "Fog Strength"
                                                 : (eidx == 6)   ? "Rim Strength"
                                                 : (eidx == 7)   ? "Rim Power"
                                                 : (eidx == 8)   ? "Exposure"
                                                                 : "Mix Amount";
                drawText(x + 20.0f, iy + 18.0f, label, 0.9f, 0.95f, 1.0f, 1.0f);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                char buf[32];
                float val = (eidx == 0) ? settings.snowGlowIntensity : (eidx == 1) ? settings.snowSparkleIntensity
                                                                   : (eidx == 2)   ? settings.snowSparkleThreshold
                                                                   : (eidx == 3)   ? settings.snowNoiseScale
                                                                   : (eidx == 4)   ? settings.snowTintStrength
                                                                   : (eidx == 5)   ? settings.snowFogStrength
                                                                   : (eidx == 6)   ? settings.snowRimStrength
                                                                   : (eidx == 7)   ? settings.snowRimPower
                                                                   : (eidx == 8)   ? settings.snowExposure
                                                                                   : settings.snowMixAmount;
                snprintf(buf, sizeof(buf), "%.2f", val);
                drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
            }
            else
            {
                // Snow material parameters
                int midx = local - baseTotal - extra;
                const char *label =
                    (midx == 0) ? "Roughness" : (midx == 1) ? "Metallic"
                                            : (midx == 2)   ? "Subsurface"
                                            : (midx == 3)   ? "Anisotropy"
                                            : (midx == 4)   ? "Base Alpha"
                                            : (midx == 5)   ? "Edge Fade"
                                            : (midx == 6)   ? "Normal Amp"
                                            : (midx == 7)   ? "Crack Scale"
                                                            : "Crack Intensity";
                drawText(x + 20.0f, iy + 18.0f, label, 0.9f, 0.95f, 1.0f, 1.0f);
                drawFilledRect(rightX, iy + 6.0f, 120.0f, itemH - 12.0f, 0.18f, 0.22f, 0.32f, 0.85f);
                char buf[32];
                float val = (midx == 0) ? settings.snowRoughness : (midx == 1) ? settings.snowMetallic
                                                               : (midx == 2)   ? settings.snowSSS
                                                               : (midx == 3)   ? settings.snowAnisotropy
                                                               : (midx == 4)   ? settings.snowBaseAlpha
                                                               : (midx == 5)   ? settings.snowEdgeFade
                                                               : (midx == 6)   ? settings.snowNormalAmplitude
                                                               : (midx == 7)   ? settings.snowCrackScale
                                                                               : settings.snowCrackIntensity;
                snprintf(buf, sizeof(buf), "%.2f", val);
                drawText(rightX + 10.0f, iy + 18.0f, buf, 0.9f, 0.95f, 1.0f, 1.0f);
            }
        }
        // Scrollbar
        int totalRows = total;
        float trackX = x + panelW - 10.0f;
        float trackY = startY;
        float trackH = panelH - 76.0f - 34.0f - 10.0f;
        drawFilledRect(trackX, trackY, 4.0f, trackH, 0.15f, 0.2f, 0.3f, 0.6f);
        float thumbH = std::max(20.0f, trackH * (visibleRows / (float)std::max(1, totalRows)));
        float thumbY = trackY + (trackH - thumbH) * (state.scrollIndex / (float)std::max(1, totalRows - visibleRows));
        drawFilledRect(trackX, thumbY, 4.0f, thumbH, 0.35f, 0.55f, 0.9f, 0.9f);
    }

    // Footer hint bar
    if (settings.debugOverlayEnabled)
    {
        drawFilledRect(x, y + panelH - 34.0f, panelW, 34.0f, 0.1f, 0.15f, 0.25f, 0.95f);
        char dbg[256];
        snprintf(dbg, sizeof(dbg), "Active:%d BVH:%d Drawn:%d Off:%d Tiny:%d Cap:%d", g_statActive, g_statBVH, g_statDrawn, g_statOff, g_statTiny, g_statCap);
        drawText(x + 14.0f, y + panelH - 12.0f, dbg, 0.85f, 0.9f, 1.0f, 1.0f);
    }

    restoreMatrices();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(prevProgram);
}
