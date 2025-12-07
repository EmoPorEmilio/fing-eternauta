#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

Shader::~Shader()
{
    if (m_program)
        glDeleteProgram(m_program);
}

Shader::Shader(Shader&& other) noexcept
    : m_program(other.m_program)
{
    other.m_program = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (m_program)
            glDeleteProgram(m_program);
        m_program = other.m_program;
        other.m_program = 0;
    }
    return *this;
}

GLuint Shader::compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed:\n" << infoLog << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool Shader::loadFromSource(const char* vertexSrc, const char* fragmentSrc)
{
    GLuint vertShader = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fragShader = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    if (!vertShader || !fragShader)
    {
        if (vertShader) glDeleteShader(vertShader);
        if (fragShader) glDeleteShader(fragShader);
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vertShader);
    glAttachShader(m_program, fragShader);
    glLinkProgram(m_program);

    GLint success;
    glGetProgramiv(m_program, GL_LINK_STATUS, &success);

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
        std::cerr << "Shader linking failed:\n" << infoLog << std::endl;
        glDeleteProgram(m_program);
        m_program = 0;
        return false;
    }

    return true;
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    auto readFile = [](const std::string& path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open())
        {
            std::cerr << "Failed to open shader file: " << path << std::endl;
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    };

    std::string vertexSrc = readFile(vertexPath);
    std::string fragmentSrc = readFile(fragmentPath);

    if (vertexSrc.empty() || fragmentSrc.empty())
        return false;

    return loadFromSource(vertexSrc.c_str(), fragmentSrc.c_str());
}

void Shader::use() const
{
    glUseProgram(m_program);
}

GLint Shader::getUniformLocation(const char* name) const
{
    return glGetUniformLocation(m_program, name);
}

void Shader::setMat4(const char* name, const glm::mat4& mat) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat4Array(const char* name, const std::vector<glm::mat4>& mats) const
{
    if (!mats.empty())
    {
        glUniformMatrix4fv(getUniformLocation(name), static_cast<GLsizei>(mats.size()), GL_FALSE, glm::value_ptr(mats[0]));
    }
}

void Shader::setVec3(const char* name, const glm::vec3& vec) const
{
    glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(vec));
}

void Shader::setInt(const char* name, int value) const
{
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const char* name, float value) const
{
    glUniform1f(getUniformLocation(name), value);
}
