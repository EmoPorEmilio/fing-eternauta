#pragma once
#include "Font.h"
#include <unordered_map>
#include <string>
#include <memory>

class FontManager {
public:
    FontManager() = default;
    ~FontManager() = default;

    bool loadFont(const std::string& id, const std::string& path, int pixelHeight) {
        std::string key = makeKey(id, pixelHeight);

        auto font = std::make_unique<Font>();
        if (!font->load(path, static_cast<float>(pixelHeight))) {
            return false;
        }

        m_fonts[key] = std::move(font);
        return true;
    }

    Font* getFont(const std::string& id, int pixelHeight) {
        std::string key = makeKey(id, pixelHeight);
        auto it = m_fonts.find(key);
        return it != m_fonts.end() ? it->second.get() : nullptr;
    }

    bool hasFont(const std::string& id, int pixelHeight) const {
        return m_fonts.count(makeKey(id, pixelHeight)) > 0;
    }

private:
    static std::string makeKey(const std::string& id, int pixelHeight) {
        return id + "@" + std::to_string(pixelHeight);
    }

    std::unordered_map<std::string, std::unique_ptr<Font>> m_fonts;
};
