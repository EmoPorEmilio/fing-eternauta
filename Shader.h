#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class Shader
{
public:
    Shader();
    ~Shader();

    bool loadFromFiles(const std::string &vertexPath, const std::string &fragmentPath);
    bool loadWithTessellation(const std::string &vertexPath,
                              const std::string &tessControlPath,
                              const std::string &tessEvalPath,
                              const std::string &fragmentPath);
    void use() const;
    void cleanup();

    void setUniform(const std::string &name, int value) const;
    void setUniform(const std::string &name, float value) const;
    void setUniform(const std::string &name, const glm::vec2 &value) const;
    void setUniform(const std::string &name, const glm::vec3 &value) const;
    void setUniform(const std::string &name, const glm::mat4 &value) const;
    void setUniform(const std::string &name, bool value) const;

    GLuint getProgram() const { return programID; }

private:
    GLuint programID;

    // Cache uniform locations to avoid glGetUniformLocation calls every frame
    mutable std::unordered_map<std::string, GLint> m_uniformCache;

    std::string loadShaderText(const std::string &fileName);
    GLuint compileShader(GLenum type, const char *src);
    GLuint createProgram(const std::string &vs, const std::string &fs);
    GLuint createProgramWithTess(const std::string &vs,
                                 const std::string &tcs,
                                 const std::string &tes,
                                 const std::string &fs);
    GLint getUniformLocation(const std::string &name) const;
};
