#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <chrono>
#include <glad/glad.h>
#include <SDL.h>

/**
 * Centralized asset management and utility functions
 * Handles cross-platform path resolution and resource loading
 */
class AssetManager
{
public:
    /**
     * Resolves asset paths across multiple potential locations
     * @param filename The asset filename (e.g., "snow.png")
     * @param subdirectory Optional subdirectory (e.g., "textures", "models")
     * @return Full path to the asset, or empty string if not found
     */
    static std::string resolveAssetPath(const std::string &filename, const std::string &subdirectory = "");

    /**
     * Loads a text file (shader, config, etc.) with proper error handling
     * @param filepath Full or relative path to the file
     * @param content Output string to store file content
     * @return true if successful, false otherwise
     */
    static bool loadTextFile(const std::string &filepath, std::string &content);

    /**
     * Shader compilation with proper error checking and reporting
     * @param type GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
     * @param source Shader source code
     * @param shaderName Name for error reporting (e.g., "vertex", "fragment")
     * @return Shader ID, or 0 if compilation failed
     */
    static GLuint compileShader(GLenum type, const char *source, const std::string &shaderName);

    /**
     * Shader program linking with proper error checking
     * @param vertexShader Compiled vertex shader ID
     * @param fragmentShader Compiled fragment shader ID
     * @param programName Name for error reporting
     * @return Program ID, or 0 if linking failed
     */
    static GLuint linkShaderProgram(GLuint vertexShader, GLuint fragmentShader, const std::string &programName);

    /**
     * Load and compile shader program from files
     * @param vertexPath Path to vertex shader file
     * @param fragmentPath Path to fragment shader file
     * @param programName Name for error reporting
     * @return Program ID, or 0 if loading/compilation failed
     */
    static GLuint loadShaderProgram(const std::string &vertexPath, const std::string &fragmentPath, const std::string &programName);

    /**
     * Check OpenGL errors and report them
     * @param operation Description of the operation that might have failed
     * @return true if no errors, false if errors were found
     */
    static bool checkGLError(const std::string &operation);

private:
    static std::vector<std::string> getSearchPaths(const std::string &subdirectory);
    static bool fileExists(const std::string &path);
};

// Performance profiling utilities
class PerformanceProfiler
{
public:
    struct ProfileData
    {
        float frameTime = 0.0f;
        float renderTime = 0.0f;
        float updateTime = 0.0f;
        int visibleObjects = 0;
        int totalObjects = 0;
        int drawCalls = 0;
        float memoryUsage = 0.0f; // MB
    };

    static void startFrame();
    static void endFrame();
    static void startTimer(const std::string &name);
    static void endTimer(const std::string &name);
    static void setCounter(const std::string &name, int value);
    static void printStats();
    static const ProfileData &getCurrentFrame() { return currentFrame; }

private:
    static ProfileData currentFrame;
    static std::chrono::high_resolution_clock::time_point frameStart;
    static std::chrono::high_resolution_clock::time_point timerStart;
    static int frameCounter;
    static constexpr int STATS_PRINT_INTERVAL = 60; // Every 60 frames
};
