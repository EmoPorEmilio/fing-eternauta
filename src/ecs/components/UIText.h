#pragma once
#include <string>
#include <glm/glm.hpp>

enum class HorizontalAlign {
    Left,
    Center,
    Right
};

enum class VerticalAlign {
    Top,
    Center,
    Bottom
};

enum class AnchorPoint {
    TopLeft,
    TopCenter,
    TopRight,
    CenterLeft,
    Center,
    CenterRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

struct UIText {
    std::string text;
    std::string fontId = "default";  // Font identifier for FontManager
    int fontSize = 24;

    // Position relative to anchor (in pixels, offset from anchor)
    glm::vec2 offset = glm::vec2(0.0f);

    // Where on the screen this element is anchored
    AnchorPoint anchor = AnchorPoint::Center;

    // Text alignment within its bounds
    HorizontalAlign horizontalAlign = HorizontalAlign::Center;

    // Color (RGBA, 0-255)
    glm::vec4 color = glm::vec4(255.0f);

    // Visibility
    bool visible = true;

    // Layer for draw ordering (higher = on top)
    int layer = 0;
};

// Helper to convert anchor point to normalized screen coordinates (0-1)
inline glm::vec2 anchorToNormalized(AnchorPoint anchor) {
    switch (anchor) {
        case AnchorPoint::TopLeft:      return {0.0f, 1.0f};
        case AnchorPoint::TopCenter:    return {0.5f, 1.0f};
        case AnchorPoint::TopRight:     return {1.0f, 1.0f};
        case AnchorPoint::CenterLeft:   return {0.0f, 0.5f};
        case AnchorPoint::Center:       return {0.5f, 0.5f};
        case AnchorPoint::CenterRight:  return {1.0f, 0.5f};
        case AnchorPoint::BottomLeft:   return {0.0f, 0.0f};
        case AnchorPoint::BottomCenter: return {0.5f, 0.0f};
        case AnchorPoint::BottomRight:  return {1.0f, 0.0f};
    }
    return {0.5f, 0.5f};
}
