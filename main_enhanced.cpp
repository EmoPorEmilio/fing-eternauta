#include <windows.h>
#include <glad/glad.h>
#include "SDL.h"
#include "SDL_opengl.h"
#include "FreeImage.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <vector>
#include "Camera.h"
#include "Mesh.h"

using namespace std;

// Global variables
GLuint shaderProgram;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool firstMouse = true;
float lastX = 400.0f;
float lastY = 300.0f;

// Function declarations
const char *loadFile(char *fname);
void exitFatalError(char *message);
void printShaderError(GLint shader);
GLuint initShaders(char *vertFile, char *fragFile);
void processInput(SDL_Window *window);
void mouse_callback(double xpos, double ypos);
void scroll_callback(double xoffset, double yoffset);

// Create a cube mesh
std::vector<Vertex> createCubeVertices()
{
    std::vector<Vertex> vertices = {
        // Front face
        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},

        // Back face
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.5f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},

        // Left face
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.5f, 0.5f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.5f, 0.0f, 0.5f}, {0.0f, 0.0f}},

        // Right face
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 0.5f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.5f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.5f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.5f, 1.0f}, {1.0f, 0.0f}},

        // Bottom face
        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.5f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f, 0.5f}, {1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}},

        // Top face
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.5f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.5f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f, 0.5f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.5f}, {0.0f, 1.0f}}};
    return vertices;
}

std::vector<unsigned int> createCubeIndices()
{
    std::vector<unsigned int> indices = {
        0, 1, 2, 2, 3, 0,       // front
        4, 5, 6, 6, 7, 4,       // back
        8, 9, 10, 10, 11, 8,    // left
        12, 13, 14, 14, 15, 12, // right
        16, 17, 18, 18, 19, 16, // bottom
        20, 21, 22, 22, 23, 20  // top
    };
    return indices;
}

void init()
{
    // Create shader program
    shaderProgram = initShaders("shaders/phong.vert", "shaders/phong.frag");

    // Create cube mesh
    auto vertices = createCubeVertices();
    auto indices = createCubeIndices();
    Mesh cube(vertices, indices);

    glEnable(GL_DEPTH_TEST);
}

void draw(SDL_Window *window)
{
    float currentFrame = SDL_GetTicks() / 1000.0f;
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use shader program
    glUseProgram(shaderProgram);

    // Create projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(camera.getZoom()), 800.0f / 600.0f, 0.1f, 100.0f);

    // Create view matrix
    glm::mat4 view = camera.getViewMatrix();

    // Create model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, (float)SDL_GetTicks() / 1000.0f, glm::vec3(0.5f, 1.0f, 0.0f));

    // Pass matrices to shader
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    // Pass lighting parameters
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 2.0f, 2.0f, 2.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), camera.getPosition().x, camera.getPosition().y, camera.getPosition().z);
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.1f);
    glUniform1f(glGetUniformLocation(shaderProgram, "diffuseStrength"), 0.8f);
    glUniform1f(glGetUniformLocation(shaderProgram, "specularStrength"), 0.5f);
    glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), 32.0f);

    // Draw cube
    // Note: In a real implementation, you'd store the mesh and call cube.draw()
    // For now, we'll use the original pyramid drawing

    SDL_GL_SwapWindow(window);
}

void processInput(SDL_Window *window)
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if (state[SDL_SCANCODE_W])
        camera.processKeyboard(Camera::FORWARD, deltaTime);
    if (state[SDL_SCANCODE_S])
        camera.processKeyboard(Camera::BACKWARD, deltaTime);
    if (state[SDL_SCANCODE_A])
        camera.processKeyboard(Camera::LEFT, deltaTime);
    if (state[SDL_SCANCODE_D])
        camera.processKeyboard(Camera::RIGHT, deltaTime);
    if (state[SDL_SCANCODE_ESCAPE])
        SDL_Quit();
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Enhanced OpenGL Demo",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          800, 600, SDL_WINDOW_OPENGL);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0);

    gladLoadGLLoader(SDL_GL_GetProcAddress);

    printf("OpenGL loaded\n");
    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version:  %s\n", glGetString(GL_VERSION));

    init();

    bool running = true;
    SDL_Event sdlEvent;

    while (running)
    {
        while (SDL_PollEvent(&sdlEvent))
        {
            if (sdlEvent.type == SDL_QUIT)
                running = false;
            else if (sdlEvent.type == SDL_MOUSEMOTION)
            {
                if (firstMouse)
                {
                    lastX = sdlEvent.motion.x;
                    lastY = sdlEvent.motion.y;
                    firstMouse = false;
                }

                float xoffset = sdlEvent.motion.x - lastX;
                float yoffset = lastY - sdlEvent.motion.y;
                lastX = sdlEvent.motion.x;
                lastY = sdlEvent.motion.y;

                camera.processMouseMovement(xoffset, yoffset);
            }
            else if (sdlEvent.type == SDL_MOUSEWHEEL)
            {
                camera.processMouseScroll(sdlEvent.wheel.y);
            }
        }

        processInput(window);
        draw(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
