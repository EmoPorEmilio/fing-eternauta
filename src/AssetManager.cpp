#include "AssetManager.h"
#include <chrono>
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <fstream>

// AssetManager Implementation
std::string AssetManager::resolveAssetPath(const std::string &filename, const std::string &subdirectory)
{
    auto searchPaths = getSearchPaths(subdirectory);

    for (const auto &basePath : searchPaths)
    {
        std::string fullPath = basePath + filename;
        if (fileExists(fullPath))
        {
            return fullPath;
        }
    }

    std::cerr << "Asset not found: " << filename;
    if (!subdirectory.empty())
    {
        std::cerr << " in subdirectory: " << subdirectory;
    }
    std::cerr << std::endl;

    return "";
}

bool AssetManager::loadTextFile(const std::string &filepath, std::string &content)
{
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    auto size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
    {
        std::cerr << "File is empty or invalid: " << filepath << std::endl;
        return false;
    }

    content.resize(static_cast<size_t>(size));
    file.read(&content[0], size);

    if (file.fail())
    {
        std::cerr << "Failed to read file: " << filepath << std::endl;
        return false;
    }

    return true;
}

GLuint AssetManager::compileShader(GLenum type, const char *source, const std::string &shaderName)
{
    GLuint shader = glCreateShader(type);
    if (shader == 0)
    {
        std::cerr << "Failed to create " << shaderName << " shader object" << std::endl;
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Check compilation status
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            std::string log(logLength, '\0');
            glGetShaderInfoLog(shader, logLength, nullptr, &log[0]);
            std::cerr << "Shader compilation failed (" << shaderName << "):\n"
                      << log << std::endl;
        }
        else
        {
            std::cerr << "Shader compilation failed (" << shaderName << ") - no log available" << std::endl;
        }

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint AssetManager::linkShaderProgram(GLuint vertexShader, GLuint fragmentShader, const std::string &programName)
{
    GLuint program = glCreateProgram();
    if (program == 0)
    {
        std::cerr << "Failed to create shader program: " << programName << std::endl;
        return 0;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Check linking status
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

        if (logLength > 0)
        {
            std::string log(logLength, '\0');
            glGetProgramInfoLog(program, logLength, nullptr, &log[0]);
            std::cerr << "Shader program linking failed (" << programName << "):\n"
                      << log << std::endl;
        }
        else
        {
            std::cerr << "Shader program linking failed (" << programName << ") - no log available" << std::endl;
        }

        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint AssetManager::loadShaderProgram(const std::string &vertexPath, const std::string &fragmentPath, const std::string &programName)
{
    // Resolve shader file paths
    std::string vertexFullPath = resolveAssetPath(vertexPath, "shaders");
    std::string fragmentFullPath = resolveAssetPath(fragmentPath, "shaders");

    if (vertexFullPath.empty() || fragmentFullPath.empty())
    {
        std::cerr << "Failed to find shader files for program: " << programName << std::endl;
        return 0;
    }

    // Load shader source code
    std::string vertexSource, fragmentSource;
    if (!loadTextFile(vertexFullPath, vertexSource) || !loadTextFile(fragmentFullPath, fragmentSource))
    {
        std::cerr << "Failed to load shader source files for program: " << programName << std::endl;
        return 0;
    }

    // Compile shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource.c_str(), programName + "_vertex");
    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource.c_str(), programName + "_fragment");
    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    // Link program
    GLuint program = linkShaderProgram(vertexShader, fragmentShader, programName);

    // Clean up individual shaders (they're now part of the program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    if (program != 0)
    {
        std::cout << "Successfully loaded shader program: " << programName << std::endl;
    }

    return program;
}

bool AssetManager::checkGLError(const std::string &operation)
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL error after " << operation << ": ";
        switch (error)
        {
        case GL_INVALID_ENUM:
            std::cerr << "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            std::cerr << "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            std::cerr << "GL_INVALID_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            std::cerr << "GL_OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            std::cerr << "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        default:
            std::cerr << "Unknown error " << error;
            break;
        }
        std::cerr << std::endl;
        return false;
    }
    return true;
}

std::vector<std::string> AssetManager::getSearchPaths(const std::string &subdirectory)
{
    std::vector<std::string> paths;

    // Base paths to search
    std::vector<std::string> basePaths = {
        "",         // Current directory
        "../",      // One level up
        "../../",   // Two levels up
        "../../../" // Three levels up
    };

    // Add SDL base path if available
    char *sdlBasePath = SDL_GetBasePath();
    if (sdlBasePath)
    {
        basePaths.push_back(std::string(sdlBasePath));
        SDL_free(sdlBasePath);
    }

    // Build full search paths
    for (const auto &basePath : basePaths)
    {
        if (subdirectory.empty())
        {
            paths.push_back(basePath);
        }
        else
        {
            // Try both forward and backward slashes for cross-platform compatibility
            paths.push_back(basePath + subdirectory + "/");
            paths.push_back(basePath + subdirectory + "\\");
        }

        // Also try with assets/ prefix
        if (!subdirectory.empty())
        {
            paths.push_back(basePath + "assets/" + subdirectory + "/");
            paths.push_back(basePath + "assets\\" + subdirectory + "\\");
        }
        else
        {
            paths.push_back(basePath + "assets/");
            paths.push_back(basePath + "assets\\");
        }
    }

    return paths;
}

bool AssetManager::fileExists(const std::string &path)
{
    std::ifstream file(path);
    return file.good();
}

// PerformanceProfiler Implementation
PerformanceProfiler::ProfileData PerformanceProfiler::currentFrame;
std::chrono::high_resolution_clock::time_point PerformanceProfiler::frameStart;
std::chrono::high_resolution_clock::time_point PerformanceProfiler::timerStart;
int PerformanceProfiler::frameCounter = 0;

void PerformanceProfiler::startFrame()
{
    PerformanceProfiler::frameStart = std::chrono::high_resolution_clock::now();
    PerformanceProfiler::currentFrame = ProfileData(); // Reset frame data
}

void PerformanceProfiler::endFrame()
{
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - PerformanceProfiler::frameStart);
    PerformanceProfiler::currentFrame.frameTime = duration.count() / 1000.0f; // Convert to milliseconds

    PerformanceProfiler::frameCounter++;
    if (PerformanceProfiler::frameCounter >= STATS_PRINT_INTERVAL)
    {
        printStats();
        PerformanceProfiler::frameCounter = 0;
    }
}

void PerformanceProfiler::startTimer(const std::string &name)
{
    PerformanceProfiler::timerStart = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::endTimer(const std::string &name)
{
    auto timerEnd = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(timerEnd - PerformanceProfiler::timerStart);
    float timeMs = duration.count() / 1000.0f;

    if (name == "render")
    {
        PerformanceProfiler::currentFrame.renderTime = timeMs;
    }
    else if (name == "update")
    {
        PerformanceProfiler::currentFrame.updateTime = timeMs;
    }
}

void PerformanceProfiler::setCounter(const std::string &name, int value)
{
    if (name == "visible_objects")
    {
        PerformanceProfiler::currentFrame.visibleObjects = value;
    }
    else if (name == "total_objects")
    {
        PerformanceProfiler::currentFrame.totalObjects = value;
    }
    else if (name == "draw_calls")
    {
        PerformanceProfiler::currentFrame.drawCalls = value;
    }
}

void PerformanceProfiler::printStats()
{
    float fps = (PerformanceProfiler::currentFrame.frameTime > 0.0f) ? (1000.0f / PerformanceProfiler::currentFrame.frameTime) : 0.0f;

    std::cout << "=== Performance Stats ===" << std::endl;
    std::cout << "FPS: " << fps << " (" << PerformanceProfiler::currentFrame.frameTime << " ms)" << std::endl;
    std::cout << "Update: " << PerformanceProfiler::currentFrame.updateTime << " ms" << std::endl;
    std::cout << "Render: " << PerformanceProfiler::currentFrame.renderTime << " ms" << std::endl;
    std::cout << "Objects: " << PerformanceProfiler::currentFrame.visibleObjects << "/" << PerformanceProfiler::currentFrame.totalObjects << std::endl;
    std::cout << "Draw Calls: " << PerformanceProfiler::currentFrame.drawCalls << std::endl;
    std::cout << "=========================" << std::endl;
}
