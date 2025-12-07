#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromSource(const char* vertexSrc, const char* fragmentSrc);
    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    void use() const;

    void setMat4(const char* name, const glm::mat4& mat) const;
    void setMat4Array(const char* name, const std::vector<glm::mat4>& mats) const;
    void setVec3(const char* name, const glm::vec3& vec) const;
    void setInt(const char* name, int value) const;
    void setFloat(const char* name, float value) const;

    GLuint getID() const { return m_program; }

private:
    GLuint m_program = 0;

    GLuint compileShader(GLenum type, const char* source);
    GLint getUniformLocation(const char* name) const;
};
