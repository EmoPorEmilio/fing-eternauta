#pragma once
#include "../Registry.h"
#include "../components/UIText.h"
#include "../../ui/FontManager.h"
#include "../../ui/TextCache.h"
#include "../../ui/UIRenderer.h"
#include <algorithm>
#include <vector>

class UISystem {
public:
    UISystem() = default;

    bool init() {
        return m_renderer.init();
    }

    void cleanup() {
        m_textCache.clear();
        m_renderer.cleanup();
    }

    FontManager& fonts() { return m_fontManager; }
    const FontManager& fonts() const { return m_fontManager; }

    void update(Registry& registry, int screenWidth, int screenHeight) {
        m_renderer.beginFrame(screenWidth, screenHeight);

        // Collect all UI text entities and sort by layer
        std::vector<std::pair<int, Entity>> sortedEntities;

        registry.forEachUIText([&](Entity entity, UIText& uiText) {
            if (uiText.visible) {
                sortedEntities.emplace_back(uiText.layer, entity);
            }
        });

        // Sort by layer (lower layers drawn first)
        std::sort(sortedEntities.begin(), sortedEntities.end(),
                  [](const auto& a, const auto& b) { return a.first < b.first; });

        // Render each text element
        for (const auto& [layer, entity] : sortedEntities) {
            auto* uiText = registry.getUIText(entity);
            if (!uiText) continue;

            // Get or load font
            Font* font = m_fontManager.getFont(uiText->fontId, uiText->fontSize);
            if (!font) continue;

            // Render text to texture (cached)
            TextStyle style;
            style.r = static_cast<uint8_t>(uiText->color.r);
            style.g = static_cast<uint8_t>(uiText->color.g);
            style.b = static_cast<uint8_t>(uiText->color.b);
            style.a = static_cast<uint8_t>(uiText->color.a);

            TextTexture texture = m_textCache.render(*font, uiText->text, style);
            if (!texture.isValid()) continue;

            m_renderer.renderText(texture, *uiText);
        }
    }

    // Invalidate cache for a specific text (call when text changes)
    void invalidateText(const std::string& text) {
        m_textCache.invalidate(text);
    }

    // Clear all cached textures
    void clearCache() {
        m_textCache.clear();
    }

private:
    FontManager m_fontManager;
    TextCache m_textCache;
    UIRenderer m_renderer;
};
