#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include <SDL_filesystem.h>

Shader::Shader() : programID(0)
{
}

Shader::~Shader()
{
    if (programID != 0)
        glDeleteProgram(programID);
}

bool Shader::loadFromFiles(const std::string &vertexPath, const std::string &fragmentPath)
{
    std::string vs = loadShaderText(vertexPath);
    std::string fs = loadShaderText(fragmentPath);

    programID = createProgram(vs, fs);
    return programID != 0;
}

bool Shader::loadWithTessellation(const std::string &vertexPath,
                                  const std::string &tessControlPath,
                                  const std::string &tessEvalPath,
                                  const std::string &fragmentPath)
{
    std::string vs = loadShaderText(vertexPath);
    std::string tcs = loadShaderText(tessControlPath);
    std::string tes = loadShaderText(tessEvalPath);
    std::string fs = loadShaderText(fragmentPath);

    programID = createProgramWithTess(vs, tcs, tes, fs);
    return programID != 0;
}

void Shader::use() const
{
    glUseProgram(programID);
}

void Shader::cleanup()
{
    if (programID != 0)
    {
        glDeleteProgram(programID);
        programID = 0;
    }
    m_uniformCache.clear();
}

void Shader::setUniform(const std::string &name, int value) const
{
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string &name, float value) const
{
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setUniform(const std::string &name, const glm::vec2 &value) const
{
    glUniform2f(getUniformLocation(name), value.x, value.y);
}

void Shader::setUniform(const std::string &name, const glm::vec3 &value) const
{
    glUniform3f(getUniformLocation(name), value.x, value.y, value.z);
}

void Shader::setUniform(const std::string &name, const glm::mat4 &value) const
{
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setUniform(const std::string &name, bool value) const
{
    glUniform1i(getUniformLocation(name), value ? 1 : 0);
}

std::string Shader::loadShaderText(const std::string &fileName)
{
    std::vector<std::string> candidates;
    // Try the provided path as-is first (supports copying shaders next to the exe)
    candidates.emplace_back(fileName);
    candidates.emplace_back(std::string("shaders/") + fileName);
    candidates.emplace_back(std::string("../shaders/") + fileName);
    candidates.emplace_back(std::string("../../shaders/") + fileName);
    candidates.emplace_back(std::string("../../../shaders/") + fileName);

    // Also try relative to the executable directory (handles VS working dir != exe dir)
    char *base = SDL_GetBasePath();
    if (base)
    {
        std::string b(base);
        SDL_free(base);
        candidates.emplace_back(b + fileName);
        candidates.emplace_back(b + std::string("shaders/") + fileName);
    }

    for (const std::string &path : candidates)
    {
        std::ifstream f(path, std::ios::in | std::ios::binary);
        if (f)
        {
            f.seekg(0, std::ios::end);
            std::streamoff size = f.tellg();
            f.seekg(0, std::ios::beg);
            std::string text(static_cast<size_t>(size), '\0');
            if (size > 0)
            {
                f.read(&text[0], static_cast<std::streamsize>(size));
            }
            return text;
        }
    }

    std::cerr << "Failed to load shader file: " << fileName << std::endl;
    return "";
}

GLuint Shader::compileShader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len);
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        std::cerr << "Shader compile error: " << log.data() << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint Shader::createProgram(const std::string &vs, const std::string &fs)
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fs.c_str());

    if (vertexShader == 0 || fragmentShader == 0)
    {
        if (vertexShader)
            glDeleteShader(vertexShader);
        if (fragmentShader)
            glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vertexShader);
    glAttachShader(prog, fragmentShader);
    glLinkProgram(prog);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Program link error: " << log.data() << std::endl;
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

GLuint Shader::createProgramWithTess(const std::string &vs,
                                     const std::string &tcs,
                                     const std::string &tes,
                                     const std::string &fs)
{
    GLuint v = compileShader(GL_VERTEX_SHADER, vs.c_str());
    GLuint tc = compileShader(GL_TESS_CONTROL_SHADER, tcs.c_str());
    GLuint te = compileShader(GL_TESS_EVALUATION_SHADER, tes.c_str());
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs.c_str());
    if (!v || !tc || !te || !f)
    {
        if (v)
            glDeleteShader(v);
        if (tc)
            glDeleteShader(tc);
        if (te)
            glDeleteShader(te);
        if (f)
            glDeleteShader(f);
        return 0;
    }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, tc);
    glAttachShader(prog, te);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    glDeleteShader(v);
    glDeleteShader(tc);
    glDeleteShader(te);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "Program link error: " << log.data() << std::endl;
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

GLint Shader::getUniformLocation(const std::string &name) const
{
    // Check cache first
    auto it = m_uniformCache.find(name);
    if (it != m_uniformCache.end())
    {
        return it->second;
    }

    // Cache miss - look up and store
    GLint location = glGetUniformLocation(programID, name.c_str());
    m_uniformCache[name] = location;
    return location;
}
