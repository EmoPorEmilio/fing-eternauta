#pragma once
#include "Font.h"
#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <unordered_map>
#include <algorithm>

struct TextTexture {
    GLuint textureId = 0;
    int width = 0;
    int height = 0;

    bool isValid() const { return textureId != 0; }

    void destroy() {
        if (textureId) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
        }
        width = height = 0;
    }
};

struct TextStyle {
    uint8_t r = 255, g = 255, b = 255, a = 255;

    bool operator==(const TextStyle& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
};

class TextCache {
public:
    TextCache() = default;

    ~TextCache() {
        clear();
    }

    // Render text to texture (returns cached version if exists)
    TextTexture render(const Font& font, const std::string& text, const TextStyle& style) {
        if (!font.isValid() || text.empty()) {
            return {};
        }

        // Check cache
        std::string key = makeCacheKey(font, text, style);
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }

        TTF_Font* ttfFont = font.handle();

        // Stroke settings
        const int outlineSize = 1;
        SDL_Color outlineColor = {0, 0, 0, 255};  // Black outline
        SDL_Color textColor = {style.r, style.g, style.b, style.a};

        // Render outline first (set outline, render, reset)
        TTF_SetFontOutline(ttfFont, outlineSize);
        SDL_Surface* outlineSurface = TTF_RenderText_Blended(ttfFont, text.c_str(), 0, outlineColor);
        TTF_SetFontOutline(ttfFont, 0);

        if (!outlineSurface) {
            return {};
        }

        // Render main text
        SDL_Surface* textSurface = TTF_RenderText_Blended(ttfFont, text.c_str(), 0, textColor);
        if (!textSurface) {
            SDL_DestroySurface(outlineSurface);
            return {};
        }

        // Composite: blit text onto outline surface
        SDL_Rect destRect;
        destRect.x = outlineSize;
        destRect.y = outlineSize;
        destRect.w = textSurface->w;
        destRect.h = textSurface->h;
        SDL_BlitSurface(textSurface, nullptr, outlineSurface, &destRect);
        SDL_DestroySurface(textSurface);

        // Convert surface to RGBA format for OpenGL
        SDL_Surface* rgbaSurface = SDL_ConvertSurface(outlineSurface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(outlineSurface);

        if (!rgbaSurface) {
            return {};
        }

        // Create OpenGL texture
        TextTexture result;
        result.width = rgbaSurface->w;
        result.height = rgbaSurface->h;

        glGenTextures(1, &result.textureId);
        glBindTexture(GL_TEXTURE_2D, result.textureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgbaSurface->w, rgbaSurface->h,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaSurface->pixels);
        glBindTexture(GL_TEXTURE_2D, 0);

        SDL_DestroySurface(rgbaSurface);

        // Cache it
        m_cache[key] = result;
        return result;
    }

    void invalidate(const std::string& text) {
        for (auto it = m_cache.begin(); it != m_cache.end();) {
            if (it->first.find(text) != std::string::npos) {
                it->second.destroy();
                it = m_cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    void clear() {
        for (auto& [key, tex] : m_cache) {
            tex.destroy();
        }
        m_cache.clear();
    }

private:
    std::string makeCacheKey(const Font& font, const std::string& text, const TextStyle& style) {
        return font.path() + "|" + std::to_string(font.pixelHeight()) + "|" +
               text + "|" +
               std::to_string(style.r) + "," +
               std::to_string(style.g) + "," +
               std::to_string(style.b) + "," +
               std::to_string(style.a);
    }

    std::unordered_map<std::string, TextTexture> m_cache;
};
