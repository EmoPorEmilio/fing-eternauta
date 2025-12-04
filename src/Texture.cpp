#include "Texture.h"
#include <SDL.h>
#include <iostream>
#include <vector>

#include "../libraries/tinygltf/stb_image.h"

Texture::Texture() : textureID(0)
{
}

Texture::~Texture()
{
    if (textureID != 0)
        glDeleteTextures(1, &textureID);
}

bool Texture::loadFromFile(const std::string &fileName, bool flipVertical, bool srgb)
{
    auto candidates = getSearchPaths(fileName);

    stbi_set_flip_vertically_on_load(flipVertical ? 1 : 0);
    int w = 0, h = 0, comp = 0;
    unsigned char *pixels = nullptr;

    for (const std::string &path : candidates)
    {
        pixels = stbi_load(path.c_str(), &w, &h, &comp, 4);
        if (pixels)
            break;
    }

    if (!pixels)
    {
        std::cerr << "Failed to load texture: " << fileName << std::endl;
        return false;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    GLenum internalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);

    return true;
}

void Texture::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

std::vector<std::string> Texture::getSearchPaths(const std::string &fileName)
{
    std::vector<std::string> candidates;
    candidates.emplace_back(std::string("assets/") + fileName);
    candidates.emplace_back(std::string("../assets/") + fileName);
    candidates.emplace_back(std::string("../../assets/") + fileName);
    candidates.emplace_back(std::string("../../../assets/") + fileName);

    char *basePath = SDL_GetBasePath();
    if (basePath)
    {
        std::string base(basePath);
        SDL_free(basePath);
        const char *ups[] = {"", "../", "../../", "../../../", "../../../../"};
        for (const char *up : ups)
        {
            candidates.emplace_back(base + up + "assets/" + fileName);
        }
    }

    return candidates;
}
