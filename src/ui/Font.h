#pragma once

#include <SDL3_ttf/SDL_ttf.h>
#include <string>

class Font {
public:
    Font() = default;

    ~Font() {
        if (m_font) {
            TTF_CloseFont(m_font);
            m_font = nullptr;
        }
    }

    bool load(const std::string& path, float pixelHeight) {
        m_font = TTF_OpenFont(path.c_str(), pixelHeight);
        if (!m_font) {
            return false;
        }

        m_pixelHeight = pixelHeight;
        m_path = path;

        // Get font metrics
        m_ascent = static_cast<float>(TTF_GetFontAscent(m_font));
        m_descent = static_cast<float>(TTF_GetFontDescent(m_font));
        m_lineHeight = static_cast<float>(TTF_GetFontLineSkip(m_font));

        return true;
    }

    // Move only
    Font(Font&& other) noexcept
        : m_font(other.m_font)
        , m_pixelHeight(other.m_pixelHeight)
        , m_ascent(other.m_ascent)
        , m_descent(other.m_descent)
        , m_lineHeight(other.m_lineHeight)
        , m_path(std::move(other.m_path)) {
        other.m_font = nullptr;
        other.m_pixelHeight = 0;
    }

    Font& operator=(Font&& other) noexcept {
        if (this != &other) {
            if (m_font) {
                TTF_CloseFont(m_font);
            }
            m_font = other.m_font;
            m_pixelHeight = other.m_pixelHeight;
            m_ascent = other.m_ascent;
            m_descent = other.m_descent;
            m_lineHeight = other.m_lineHeight;
            m_path = std::move(other.m_path);
            other.m_font = nullptr;
            other.m_pixelHeight = 0;
        }
        return *this;
    }

    // No copy
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    bool isValid() const { return m_font != nullptr; }
    float pixelHeight() const { return m_pixelHeight; }
    float ascent() const { return m_ascent; }
    float descent() const { return m_descent; }
    float lineHeight() const { return m_lineHeight; }
    const std::string& path() const { return m_path; }
    TTF_Font* handle() const { return m_font; }

private:
    TTF_Font* m_font = nullptr;
    float m_pixelHeight = 0.0f;
    float m_ascent = 0.0f;
    float m_descent = 0.0f;
    float m_lineHeight = 0.0f;
    std::string m_path;
};
