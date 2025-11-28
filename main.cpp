// main.cpp (barebones triangle + Phong)
// Minimal SDL2 + GLAD setup rendering a single lit triangle

#include <SDL.h>
#include <glad/glad.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>

// Texture loading via stb_image bundled in libraries/tinygltf
#define STB_IMAGE_IMPLEMENTATION
#include "libraries/tinygltf/stb_image.h"

// ------- File I/O helpers -------
static bool readFileToString(const std::vector<std::string> &candidates, std::string &out)
{
    for (const std::string &path : candidates)
    {
        std::ifstream f(path, std::ios::in | std::ios::binary);
        if (f)
        {
            f.seekg(0, std::ios::end);
            std::streamoff size = f.tellg();
            f.seekg(0, std::ios::beg);
            out.resize(static_cast<size_t>(size));
            if (size > 0)
            {
                f.read(&out[0], static_cast<std::streamsize>(size));
            }
            return true;
        }
    }
    return false;
}

static std::string loadShaderText(const char *fileName)
{
    std::vector<std::string> candidates;
    candidates.emplace_back(std::string("shaders/") + fileName);
    candidates.emplace_back(std::string("../shaders/") + fileName);
    candidates.emplace_back(std::string("../../shaders/") + fileName);
    candidates.emplace_back(std::string("../../../shaders/") + fileName);

    char *basePath = SDL_GetBasePath();
    if (basePath)
    {
        std::string base(basePath);
        SDL_free(basePath);
        const char *ups[] = {"", "../", "../../", "../../../", "../../../../"};
        for (const char *up : ups)
        {
            candidates.emplace_back(base + up + "shaders/" + fileName);
        }
    }

    std::string text;
    if (!readFileToString(candidates, text))
    {
        std::cerr << "Failed to load shader file: " << fileName << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return text;
}

// ------- Texture loader (stb_image) -------
static GLuint loadTexture2D(const char *fileName, bool flipVertical)
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

    stbi_set_flip_vertically_on_load(flipVertical ? 1 : 0);
    int w = 0, h = 0, comp = 0;
    unsigned char *pixels = nullptr;
    for (const std::string &p : candidates)
    {
        pixels = stbi_load(p.c_str(), &w, &h, &comp, 4);
        if (pixels)
            break;
    }
    if (!pixels)
    {
        std::cerr << "Failed to load texture: " << fileName << std::endl;
        std::exit(EXIT_FAILURE);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);
    return tex;
}

// Minimal matrix helpers (column-major, column vectors)
static void make_perspective(float out[16], float fovyRadians, float aspect, float znear, float zfar)
{
    const float f = 1.0f / std::tan(fovyRadians * 0.5f);
    out[0] = f / aspect;
    out[4] = 0.0f;
    out[8] = 0.0f;
    out[12] = 0.0f;
    out[1] = 0.0f;
    out[5] = f;
    out[9] = 0.0f;
    out[13] = 0.0f;
    out[2] = 0.0f;
    out[6] = 0.0f;
    out[10] = (zfar + znear) / (znear - zfar);
    out[14] = (2.0f * zfar * znear) / (znear - zfar);
    out[3] = 0.0f;
    out[7] = 0.0f;
    out[11] = -1.0f;
    out[15] = 0.0f;
}

static void make_translation(float out[16], float tx, float ty, float tz)
{
    out[0] = 1.0f;
    out[4] = 0.0f;
    out[8] = 0.0f;
    out[12] = tx;
    out[1] = 0.0f;
    out[5] = 1.0f;
    out[9] = 0.0f;
    out[13] = ty;
    out[2] = 0.0f;
    out[6] = 0.0f;
    out[10] = 1.0f;
    out[14] = tz;
    out[3] = 0.0f;
    out[7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

static void make_rotation_y(float out[16], float radians)
{
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    out[0] = c;
    out[4] = 0.0f;
    out[8] = s;
    out[12] = 0.0f;
    out[1] = 0.0f;
    out[5] = 1.0f;
    out[9] = 0.0f;
    out[13] = 0.0f;
    out[2] = -s;
    out[6] = 0.0f;
    out[10] = c;
    out[14] = 0.0f;
    out[3] = 0.0f;
    out[7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

static void make_identity(float out[16])
{
    out[0] = 1.0f;
    out[4] = 0.0f;
    out[8] = 0.0f;
    out[12] = 0.0f;
    out[1] = 0.0f;
    out[5] = 1.0f;
    out[9] = 0.0f;
    out[13] = 0.0f;
    out[2] = 0.0f;
    out[6] = 0.0f;
    out[10] = 1.0f;
    out[14] = 0.0f;
    out[3] = 0.0f;
    out[7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

// GLSL now loaded from external files in shaders/ (phong.vert, phong.frag)

static void sdlFail(const char *msg)
{
    std::cerr << msg << ": " << SDL_GetError() << std::endl;
    std::exit(EXIT_FAILURE);
}

static GLuint compileShader(GLenum type, const char *src)
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
        std::exit(EXIT_FAILURE);
    }
    return shader;
}

static GLuint createProgramFromSource(const char *vs, const char *fs)
{
    GLuint v = compileShader(GL_VERTEX_SHADER, vs);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    glDeleteShader(v);
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
        std::exit(EXIT_FAILURE);
    }
    return prog;
}

static GLuint createProgramFromFiles(const char *vsPath, const char *fsPath)
{
    const std::string vs = loadShaderText(vsPath);
    const std::string fs = loadShaderText(fsPath);
    return createProgramFromSource(vs.c_str(), fs.c_str());
}

int main(int /*argc*/, char ** /*argv*/)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdlFail("SDL_Init failed");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window *window = SDL_CreateWindow(
        "OpenGL Barebones",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        960, 540,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window)
        sdlFail("SDL_CreateWindow failed");

    SDL_GLContext glctx = SDL_GL_CreateContext(window);
    if (!glctx)
        sdlFail("SDL_GL_CreateContext failed");
    SDL_GL_MakeCurrent(window, glctx);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return EXIT_FAILURE;
    }

    int fbw = 0, fbh = 0;
    SDL_GL_GetDrawableSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);
    glEnable(GL_DEPTH_TEST);

    std::cout << "GL Vendor:   " << glGetString(GL_VENDOR) << "\n";
    std::cout << "GL Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "GL Version:  " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL:        " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Geometry: floor plane (Y=0) sized 2000x2000 with heavy UV tiling (two triangles)
    // Layout per-vertex: position(3), normal(3), uv(2)
    float vertices[] = {
        //        position                 normal           uv (tile 200x200)
        -1000.0f,
        0.0f,
        -1000.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1000.0f,
        0.0f,
        -1000.0f,
        0.0f,
        1.0f,
        0.0f,
        200.0f,
        0.0f,
        1000.0f,
        0.0f,
        1000.0f,
        0.0f,
        1.0f,
        0.0f,
        200.0f,
        200.0f,

        -1000.0f,
        0.0f,
        -1000.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1000.0f,
        0.0f,
        1000.0f,
        0.0f,
        1.0f,
        0.0f,
        200.0f,
        200.0f,
        -1000.0f,
        0.0f,
        1000.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        200.0f,
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    glBindVertexArray(0);

    GLuint program = createProgramFromFiles("phong.vert", "phong.frag");
    glUseProgram(program);

    // Uniforms (static for this demo)
    GLint locModel = glGetUniformLocation(program, "uModel");
    GLint locView = glGetUniformLocation(program, "uView");
    GLint locProj = glGetUniformLocation(program, "uProj");
    GLint locLightPos = glGetUniformLocation(program, "uLightPos");
    GLint locViewPos = glGetUniformLocation(program, "uViewPos");
    GLint locLightColor = glGetUniformLocation(program, "uLightColor");
    GLint locObjectColor = glGetUniformLocation(program, "uObjectColor");
    GLint locTex = glGetUniformLocation(program, "uTex");
    GLint locCullDist = glGetUniformLocation(program, "uCullDistance");

    // Set static projection; compute view each frame from camera
    float view[16];
    float proj[16];
    make_perspective(proj, 60.0f * 3.14159265f / 180.0f, (float)fbw / (float)fbh, 0.1f, 2000.0f);
    glUniformMatrix4fv(locProj, 1, GL_FALSE, proj);

    glUniform3f(locLightPos, 2.0f, 4.0f, 2.0f);
    glUniform3f(locLightColor, 1.0f, 1.0f, 1.0f);
    glUniform3f(locObjectColor, 1.0f, 1.0f, 1.0f);

    // Load texture and bind to texture unit 0
    GLuint tex = loadTexture2D("Baked_snowflake.png", true);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(locTex, 0);
    glUniform1f(locCullDist, 400.0f); // set <= 0 to disable culling

    // Camera state (first person, fixed yaw=0 so forward=-Z, right=+X)
    bool running = true;
    float camX = 0.0f, camY = 1.6f, camZ = 3.0f;
    Uint64 prevTicks = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();
    // Overlay shader for standalone path
    GLuint overlayProg = createProgramFromFiles("fullscreen_quad.vert", "shadertoy_overlay.frag");
    GLuint fsVAO = 0;
    glGenVertexArrays(1, &fsVAO);

    GLint loc_iResolution = glGetUniformLocation(overlayProg, "iResolution");
    GLint loc_iTime = glGetUniformLocation(overlayProg, "iTime");
    GLint loc_uSnowSpeed = glGetUniformLocation(overlayProg, "uSnowSpeed");
    GLint loc_uSnowDirectionDeg = glGetUniformLocation(overlayProg, "uSnowDirectionDeg");

    float elapsed = 0.0f;

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            else if (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED || e.window.event == SDL_WINDOWEVENT_RESIZED))
            {
                int w = 0, h = 0;
                SDL_GL_GetDrawableSize(window, &w, &h);
                fbw = w;
                fbh = h;
                glViewport(0, 0, w, h);
                // Rebuild projection for new aspect
                make_perspective(proj, 60.0f * 3.14159265f / 180.0f, (float)w / (float)h, 0.1f, 2000.0f);
                glUniformMatrix4fv(locProj, 1, GL_FALSE, proj);
            }
        }

        // Delta time
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)((now - prevTicks) / freq);
        prevTicks = now;

        // WASD movement on XZ plane
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);
        const float moveSpeed = 3.0f; // units per second
        if (keys[SDL_SCANCODE_W])
            camZ -= moveSpeed * dt; // forward (-Z)
        if (keys[SDL_SCANCODE_S])
            camZ += moveSpeed * dt; // back (+Z)
        if (keys[SDL_SCANCODE_A])
            camX -= moveSpeed * dt; // left (-X)
        if (keys[SDL_SCANCODE_D])
            camX += moveSpeed * dt; // right (+X)

        // Build view from camera position (no rotation)
        make_translation(view, -camX, -camY, -camZ);
        glUniformMatrix4fv(locView, 1, GL_FALSE, view);
        glUniform3f(locViewPos, camX, camY, camZ);

        glClearColor(0.08f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        // Static model for floor
        float model[16];
        make_identity(model);
        glUniformMatrix4fv(locModel, 1, GL_FALSE, model);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // Draw overlay fullscreen on top (alpha blended)
        elapsed += dt;
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(overlayProg);
        glUniform3f(loc_iResolution, (float)fbw, (float)fbh, 1.0f);
        glUniform1f(loc_iTime, elapsed);
        glUniform1f(loc_uSnowSpeed, 1.5f);
        glUniform1f(loc_uSnowDirectionDeg, 270.0f);
        glBindVertexArray(fsVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);

        SDL_GL_SwapWindow(window);
    }

    glDeleteProgram(overlayProg);
    glDeleteVertexArrays(1, &fsVAO);
    glDeleteProgram(program);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    SDL_GL_DeleteContext(glctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
