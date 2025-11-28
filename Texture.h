#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

class Texture
{
public:
    Texture();
    ~Texture();

    bool loadFromFile(const std::string &fileName, bool flipVertical = true, bool srgb = false);
    void bind(unsigned int unit = 0) const;

    GLuint getID() const { return textureID; }

private:
    GLuint textureID;

    std::vector<std::string> getSearchPaths(const std::string &fileName);
};
