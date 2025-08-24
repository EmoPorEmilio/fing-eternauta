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
#include <algorithm>
#include <random>

#include "Settings.h"
#include "UI.h"

using namespace std;

// Debug logging helpers
#define DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl
#define DEBUG_LOG_VALUE(name, value)                                   \
	{                                                                  \
		std::cout << "[DEBUG] " << name << ": " << value << std::endl; \
	}
#define DEBUG_SEPARATOR std::cout << "[DEBUG] ======================================" << std::endl

// OpenGL error checking
#define CHECK_GL_ERROR()                                                                            \
	{                                                                                               \
		GLenum err = glGetError();                                                                  \
		if (err != GL_NO_ERROR)                                                                     \
		{                                                                                           \
			std::cout << "[GL ERROR] " << __FILE__ << ":" << __LINE__ << " - " << err << std::endl; \
		}                                                                                           \
	}

// Frustum culling
struct Frustum
{
	glm::vec4 planes[6]; // left, right, bottom, top, near, far
};

// BVH Node
struct BVHNode
{
	glm::vec3 min, max;
	int leftChild, rightChild;
	int firstPrimitive, primitiveCount;
	bool isLeaf;
};

// Scene management
const int MAX_PYRAMIDS = 20000; // scale up 20x

// BVH tree
BVHNode bvhNodes[MAX_PYRAMIDS * 2];
int bvhNodeCount = 0;

// global variables - normally would avoid globals, using in this demo
GLuint shaderprogram;				// handle for shader program
GLuint vao, vbo[2];					// handles for our VAO and two VBOs
GLuint vaoStatic, vboStatic[2];		// static quad (floor/ceiling)
GLuint vaoImpostor, vboImpostor[2]; // camera-facing quad for impostor spheres
int impostorVertexCount = 4;
int staticVertexCount = 6;
float r = 0;

// App settings and UI state
AppSettings gSettings;
UIState gUIState;

// Per-frame performance stats for debug overlay
struct PerfStats
{
	int active = 0;
	int bvhVisible = 0;
	int drawn = 0;
	int culledOffscreen = 0;
	int culledTiny = 0;
	int budgetCapHits = 0;
};

static void renderImpostorPass(SDL_Window *window,
							   const glm::mat4 &projection,
							   const glm::mat4 &view,
							   const CadencePreset &cp,
							   float fallMul,
							   float rotMul,
							   float deltaTime,
							   const std::vector<int> &visiblePyramids,
							   PerfStats &stats);

// Performance monitoring
int totalFrames = 0;
int totalPyramidsRendered = 0;
struct Pyramid
{
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotationAxis;
	float rotationSpeed;
	glm::vec3 color;
	bool visible;
	bool landed;
	float landedTimer;
	float fallDistanceRemaining;
	glm::vec3 boundingBoxMin;
	glm::vec3 boundingBoxMax;
};

Pyramid pyramids[MAX_PYRAMIDS];
int pyramidCount = 0;					   // pool size
int gActivePyramids = 0;				   // currently active (first N)
int gTargetActivePyramids = 0;			   // target active count based on cadence
int gPendingDeactivations = 0;			   // number of actives to retire when they exit
static std::vector<char> gDeactivateFlags; // per-index flag for pending deactivation

// Cadence state
static float gCadenceTimer = 0.0f;
static float gGustTimer = 0.0f;
static float gGustElapsed = 0.0f;
static bool gGustActive = false;

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 30.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastX = 400.0f;
float lastY = 300.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool mouseCaptured = false;

// Culling optimization globals
static bool gBvhValid = false;			  // Track if BVH needs rebuild
static float gLastFOV = 80.0f;			  // Cache for screen-space calculations
static int gLastWindowWidth = 800;		  // Cache for screen-space calculations
static int gLastWindowHeight = 600;		  // Cache for screen-space calculations
static float gPixelsPerUnitY = 0.0f;	  // Precomputed camera constant
static float gMaxRenderDistance = 200.0f; // Maximum render distance for early culling
static float gMinScreenPixels = 0.5f;	  // Minimum screen pixels to render

// Performance stats for debugging
struct CullingStats
{
	int bvhTraversalTime = 0;
	int screenSpaceCullingTime = 0;
	int distanceCullingTime = 0;
	int totalCulled = 0;
	int totalProcessed = 0;
};

// RNG helper
static std::mt19937 gRng;
static bool gRngInit = false;
static float frand(float a, float b)
{
	if (!gRngInit)
	{
		std::random_device rd;
		gRng.seed(rd());
		gRngInit = true;
	}
	std::uniform_real_distribution<float> d(a, b);
	return d(gRng);
}

// loadFile - loads text file into char* fname
// allocates memory - so need to delete after use
const char *loadFile(char *fname)
{
	int size;
	char *memblock = NULL;

	// file read based on example in cplusplus.com tutorial
	// ios::ate opens file at the end
	ifstream file(fname, ios::in | ios::binary | ios::ate);
	if (file.is_open())
	{
		size = (int)file.tellg();	   // get location of file pointer i.e. file size
		memblock = new char[size + 1]; // create buffer with space for null char
		file.seekg(0, ios::beg);
		file.read(memblock, size);
		file.close();
		memblock[size] = 0;
		cout << "file " << fname << " loaded" << endl;
	}
	else
	{
		cout << "Unable to open file " << fname << endl;
		// should ideally set a flag or use exception handling
		// so that calling function can decide what to do now
	}
	return memblock;
}

// Something went wrong - print SDL error message and quit
void exitFatalError(char *message)
{
	cout << message << " ";
	cout << SDL_GetError();
	SDL_Quit();
	exit(1);
}

// Camera control functions
void processKeyboard()
{
	const Uint8 *state = SDL_GetKeyboardState(NULL);
	float cameraSpeed = gSettings.cameraSpeed * deltaTime;

	if (state[SDL_SCANCODE_W])
		cameraPos += cameraSpeed * cameraFront;
	if (state[SDL_SCANCODE_S])
		cameraPos -= cameraSpeed * cameraFront;
	if (state[SDL_SCANCODE_A])
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	if (state[SDL_SCANCODE_D])
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

	// Additional controls
	if (state[SDL_SCANCODE_SPACE])
		cameraPos += cameraSpeed * cameraUp;
	if (state[SDL_SCANCODE_LSHIFT])
		cameraPos -= cameraSpeed * cameraUp;
}

void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
{
	float sensitivity = gSettings.mouseSensitivity;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (constrainPitch)
	{
		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

// Frustum culling functions
void extractFrustumPlanes(const glm::mat4 &viewProjection, Frustum &frustum)
{
	glm::mat4 vp = viewProjection;

	// Left plane
	frustum.planes[0].x = vp[0][3] + vp[0][0];
	frustum.planes[0].y = vp[1][3] + vp[1][0];
	frustum.planes[0].z = vp[2][3] + vp[2][0];
	frustum.planes[0].w = vp[3][3] + vp[3][0];

	// Right plane
	frustum.planes[1].x = vp[0][3] - vp[0][0];
	frustum.planes[1].y = vp[1][3] - vp[1][0];
	frustum.planes[1].z = vp[2][3] - vp[2][0];
	frustum.planes[1].w = vp[3][3] - vp[3][0];

	// Bottom plane
	frustum.planes[2].x = vp[0][3] + vp[0][1];
	frustum.planes[2].y = vp[1][3] + vp[1][1];
	frustum.planes[2].z = vp[2][3] + vp[2][1];
	frustum.planes[2].w = vp[3][3] + vp[3][1];

	// Top plane
	frustum.planes[3].x = vp[0][3] - vp[0][1];
	frustum.planes[3].y = vp[1][3] - vp[1][1];
	frustum.planes[3].z = vp[2][3] - vp[2][1];
	frustum.planes[3].w = vp[3][3] - vp[3][1];

	// Near plane
	frustum.planes[4].x = vp[0][3] + vp[0][2];
	frustum.planes[4].y = vp[1][3] + vp[1][2];
	frustum.planes[4].z = vp[2][3] + vp[2][2];
	frustum.planes[4].w = vp[3][3] + vp[3][2];

	// Far plane
	frustum.planes[5].x = vp[0][3] - vp[0][2];
	frustum.planes[5].y = vp[1][3] - vp[1][2];
	frustum.planes[5].z = vp[2][3] - vp[2][2];
	frustum.planes[5].w = vp[3][3] - vp[3][2];

	// Normalize all planes
	for (int i = 0; i < 6; i++)
	{
		float length = glm::length(glm::vec3(frustum.planes[i]));
		frustum.planes[i] /= length;
	}
}

bool isAABBInFrustum(const glm::vec3 &min, const glm::vec3 &max, const Frustum &frustum)
{
	for (int i = 0; i < 6; i++)
	{
		glm::vec3 positive = max;
		glm::vec3 negative = min;

		if (frustum.planes[i].x >= 0)
		{
			positive.x = max.x;
			negative.x = min.x;
		}
		else
		{
			positive.x = min.x;
			negative.x = max.x;
		}

		if (frustum.planes[i].y >= 0)
		{
			positive.y = max.y;
			negative.y = min.y;
		}
		else
		{
			positive.y = min.y;
			negative.y = max.y;
		}

		if (frustum.planes[i].z >= 0)
		{
			positive.z = max.z;
			negative.z = min.z;
		}
		else
		{
			positive.z = min.z;
			negative.z = max.z;
		}

		if (glm::dot(glm::vec3(frustum.planes[i]), positive) + frustum.planes[i].w < 0)
		{
			return false;
		}
	}
	return true;
}

// BVH functions
int buildBVH(int start, int end, int depth = 0)
{
	if (end - start <= 1 || depth > 10)
	{
		// Create leaf node
		int nodeIndex = bvhNodeCount++;
		bvhNodes[nodeIndex].isLeaf = true;
		bvhNodes[nodeIndex].firstPrimitive = start;
		bvhNodes[nodeIndex].primitiveCount = end - start;

		// Calculate bounding box
		bvhNodes[nodeIndex].min = pyramids[start].boundingBoxMin;
		bvhNodes[nodeIndex].max = pyramids[start].boundingBoxMax;

		for (int i = start + 1; i < end; i++)
		{
			bvhNodes[nodeIndex].min.x = (bvhNodes[nodeIndex].min.x < pyramids[i].boundingBoxMin.x) ? bvhNodes[nodeIndex].min.x : pyramids[i].boundingBoxMin.x;
			bvhNodes[nodeIndex].min.y = (bvhNodes[nodeIndex].min.y < pyramids[i].boundingBoxMin.y) ? bvhNodes[nodeIndex].min.y : pyramids[i].boundingBoxMin.y;
			bvhNodes[nodeIndex].min.z = (bvhNodes[nodeIndex].min.z < pyramids[i].boundingBoxMin.z) ? bvhNodes[nodeIndex].min.z : pyramids[i].boundingBoxMin.z;
			bvhNodes[nodeIndex].max.x = (bvhNodes[nodeIndex].max.x > pyramids[i].boundingBoxMax.x) ? bvhNodes[nodeIndex].max.x : pyramids[i].boundingBoxMax.x;
			bvhNodes[nodeIndex].max.y = (bvhNodes[nodeIndex].max.y > pyramids[i].boundingBoxMax.y) ? bvhNodes[nodeIndex].max.y : pyramids[i].boundingBoxMax.y;
			bvhNodes[nodeIndex].max.z = (bvhNodes[nodeIndex].max.z > pyramids[i].boundingBoxMax.z) ? bvhNodes[nodeIndex].max.z : pyramids[i].boundingBoxMax.z;
		}

		return nodeIndex;
	}

	// Find center of bounding box
	glm::vec3 center(0.0f);
	for (int i = start; i < end; i++)
	{
		center += (pyramids[i].boundingBoxMin + pyramids[i].boundingBoxMax) * 0.5f;
	}
	center /= (end - start);

	// Sort along longest axis
	glm::vec3 extent = glm::vec3(0.0f);
	for (int i = start; i < end; i++)
	{
		glm::vec3 boxCenter = (pyramids[i].boundingBoxMin + pyramids[i].boundingBoxMax) * 0.5f;
		glm::vec3 diff = glm::abs(boxCenter - center);
		extent.x = (extent.x > diff.x) ? extent.x : diff.x;
		extent.y = (extent.y > diff.y) ? extent.y : diff.y;
		extent.z = (extent.z > diff.z) ? extent.z : diff.z;
	}

	int axis = 0;
	if (extent.y > extent.x)
		axis = 1;
	if (extent.z > extent[axis])
		axis = 2;

	// Sort primitives
	int mid = (start + end) / 2;
	std::nth_element(pyramids + start, pyramids + mid, pyramids + end,
					 [axis, center](const Pyramid &a, const Pyramid &b)
					 {
						 glm::vec3 aCenter = (a.boundingBoxMin + a.boundingBoxMax) * 0.5f;
						 glm::vec3 bCenter = (b.boundingBoxMin + b.boundingBoxMax) * 0.5f;
						 return aCenter[axis] < bCenter[axis];
					 });

	// Create internal node
	int nodeIndex = bvhNodeCount++;
	bvhNodes[nodeIndex].isLeaf = false;
	bvhNodes[nodeIndex].leftChild = buildBVH(start, mid, depth + 1);
	bvhNodes[nodeIndex].rightChild = buildBVH(mid, end, depth + 1);

	// Calculate bounding box
	int leftChild = bvhNodes[nodeIndex].leftChild;
	int rightChild = bvhNodes[nodeIndex].rightChild;

	bvhNodes[nodeIndex].min.x = (bvhNodes[leftChild].min.x < bvhNodes[rightChild].min.x) ? bvhNodes[leftChild].min.x : bvhNodes[rightChild].min.x;
	bvhNodes[nodeIndex].min.y = (bvhNodes[leftChild].min.y < bvhNodes[rightChild].min.y) ? bvhNodes[leftChild].min.y : bvhNodes[rightChild].min.y;
	bvhNodes[nodeIndex].min.z = (bvhNodes[leftChild].min.z < bvhNodes[rightChild].min.z) ? bvhNodes[leftChild].min.z : bvhNodes[rightChild].min.z;
	bvhNodes[nodeIndex].max.x = (bvhNodes[leftChild].max.x > bvhNodes[rightChild].max.x) ? bvhNodes[leftChild].max.x : bvhNodes[rightChild].max.x;
	bvhNodes[nodeIndex].max.y = (bvhNodes[leftChild].max.y > bvhNodes[rightChild].max.y) ? bvhNodes[leftChild].max.y : bvhNodes[rightChild].max.y;
	bvhNodes[nodeIndex].max.z = (bvhNodes[leftChild].max.z > bvhNodes[rightChild].max.z) ? bvhNodes[leftChild].max.z : bvhNodes[rightChild].max.z;

	return nodeIndex;
}

void traverseBVH(int nodeIndex, const Frustum &frustum, std::vector<int> &visiblePyramids)
{
	if (nodeIndex < 0 || nodeIndex >= bvhNodeCount)
		return;

	BVHNode &node = bvhNodes[nodeIndex];

	// Check if node bounding box is in frustum
	if (!isAABBInFrustum(node.min, node.max, frustum))
	{
		return;
	}

	if (node.isLeaf)
	{
		// Add all primitives in this leaf
		for (int i = 0; i < node.primitiveCount; i++)
		{
			int pyramidIndex = node.firstPrimitive + i;
			if (pyramidIndex < gActivePyramids)
			{
				visiblePyramids.push_back(pyramidIndex);
			}
		}
	}
	else
	{
		// Traverse children
		traverseBVH(node.leftChild, frustum, visiblePyramids);
		traverseBVH(node.rightChild, frustum, visiblePyramids);
	}
}

// printShaderError
// Display (hopefully) useful error messages if shader fails to compile or link
void printShaderError(GLint shader)
{
	int maxLength = 0;
	int logLength = 0;
	GLchar *logMessage;

	// Find out how long the error message is
	if (glIsShader(shader))
		glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
	else
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

	if (maxLength > 0) // If message has length > 0
	{
		logMessage = new GLchar[maxLength];
		if (glIsShader(shader))
			glGetProgramInfoLog(shader, maxLength, &logLength, logMessage);
		else
			glGetShaderInfoLog(shader, maxLength, &logLength, logMessage);
		cout << "Shader Info Log:" << endl
			 << logMessage << endl;
		delete[] logMessage;
	}
}

GLuint initShaders(char *vertFile, char *fragFile)
{
	GLuint p, f, v; // Handles for shader program & vertex and fragment shaders

	v = glCreateShader(GL_VERTEX_SHADER);	// Create vertex shader handle
	f = glCreateShader(GL_FRAGMENT_SHADER); // " fragment shader handle

	const char *vertSource = loadFile(vertFile); // load vertex shader source
	const char *fragSource = loadFile(fragFile); // load frag shader source

	// Check if shader files were loaded successfully
	if (!vertSource || !fragSource)
	{
		cout << "Failed to load shader files!" << endl;
		return 0;
	}

	// Send the shader source to the GPU
	// Strings here are null terminated - a non-zero final parameter can be
	// used to indicate the length of the shader source instead
	glShaderSource(v, 1, &vertSource, 0);
	glShaderSource(f, 1, &fragSource, 0);

	GLint compiled, linked; // return values for checking for compile & link errors

	// compile the vertex shader and test for errors
	glCompileShader(v);
	glGetShaderiv(v, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		cout << "Vertex shader not compiled." << endl;
		printShaderError(v);
	}

	// compile the fragment shader and test for errors
	glCompileShader(f);
	glGetShaderiv(f, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		cout << "Fragment shader not compiled." << endl;
		printShaderError(f);
	}

	p = glCreateProgram(); // create the handle for the shader program
	glAttachShader(p, v);  // attach vertex shader to program
	glAttachShader(p, f);  // attach fragment shader to program

	glBindAttribLocation(p, 0, "in_Position"); // bind position attribute to location 0
	glBindAttribLocation(p, 1, "in_Color");	   // bind color attribute to location 1

	glLinkProgram(p); // link the shader program and test for errors
	glGetProgramiv(p, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		cout << "Program not linked." << endl;
		printShaderError(p);
	}

	glUseProgram(p); // Make the shader program the current active program

	// We can delete the shaders after linking; the program keeps them
	glDeleteShader(v);
	glDeleteShader(f);

	delete[] vertSource; // Don't forget to free allocated memory
	delete[] fragSource; // We allocated this in the loadFile function...

	return p; // Return the shader program handle
}

void generatePyramids(int desiredCount)
{
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
	std::uniform_real_distribution<float> heightDist(0.0f, 50.0f);
	std::uniform_real_distribution<float> scaleDist(0.1f, 2.0f); // back to pyramid sizes
	std::uniform_real_distribution<float> speedDist(0.1f, 2.0f);
	std::uniform_real_distribution<float> snowColorDist(0.8f, 1.0f);

	pyramidCount = 0;

	int countToGenerate = desiredCount;
	if (countToGenerate < 0)
		countToGenerate = 0;
	if (countToGenerate > MAX_PYRAMIDS)
		countToGenerate = MAX_PYRAMIDS;

	for (int i = 0; i < countToGenerate && pyramidCount < MAX_PYRAMIDS; i++)
	{
		pyramids[pyramidCount].position = glm::vec3(posDist(gen), heightDist(gen), posDist(gen));
		pyramids[pyramidCount].scale = glm::vec3(scaleDist(gen));
		pyramids[pyramidCount].rotationAxis = glm::normalize(glm::vec3(posDist(gen), posDist(gen), posDist(gen)));
		pyramids[pyramidCount].rotationSpeed = speedDist(gen);

		float snowColor = snowColorDist(gen);
		pyramids[pyramidCount].color = glm::vec3(snowColor);
		pyramids[pyramidCount].visible = true;
		pyramids[pyramidCount].landed = false;
		pyramids[pyramidCount].landedTimer = 0.0f;
		pyramids[pyramidCount].fallDistanceRemaining = frand(8.0f, 20.0f);

		float maxScale = pyramids[pyramidCount].scale.x;
		glm::vec3 halfSize = glm::vec3(maxScale);
		pyramids[pyramidCount].boundingBoxMin = pyramids[pyramidCount].position - halfSize;
		pyramids[pyramidCount].boundingBoxMax = pyramids[pyramidCount].position + halfSize;

		pyramidCount++;
	}

	cout << "Generated pool of " << pyramidCount << " pyramids" << endl;
}

static void rebuildBVHForActive()
{
	// Lazy BVH rebuilding - only rebuild when necessary
	if (gBvhValid)
		return;

	bvhNodeCount = 0;
	if (gActivePyramids > 0)
	{
		buildBVH(0, gActivePyramids);
	}
	gBvhValid = true;
}

static void invalidateBVH()
{
	gBvhValid = false;
}

static void applyCadenceIfNeeded(float dt)
{
	// Determine target preset
	int presetIdx = 0;
	if (gSettings.cadenceSelection == CadenceSelection::One)
		presetIdx = 0;
	else if (gSettings.cadenceSelection == CadenceSelection::Two)
		presetIdx = 1;
	else if (gSettings.cadenceSelection == CadenceSelection::Three)
		presetIdx = 2;
	else
	{
		// cycle
		int cycleIndex = ((int)(gCadenceTimer / gSettings.cadenceCycleSeconds)) % 3;
		presetIdx = cycleIndex;
	}
	const CadencePreset &cp = gSettings.cadence[presetIdx];
	// Dynamically vary target actives from 0..cp.pyramids based on time and fall speed
	float speedNorm = glm::clamp(cp.fallSpeed / 10.0f, 0.0f, 1.0f);
	float wave = 0.5f + 0.5f * sinf(gCadenceTimer * (0.5f + cp.fallSpeed * 0.2f));
	int dynamicTarget = (int)(cp.pyramids * wave);
	gTargetActivePyramids = std::max(0, std::min(MAX_PYRAMIDS, dynamicTarget));
	int delta = gTargetActivePyramids - gActivePyramids;
	if (delta > 0)
	{
		// Activate a few per frame from the pool, placing them above the top so they fall in
		// Faster ramp-up scaled by fall speed
		int step = (int)ceilf(delta * (0.8f + cp.fallSpeed * 0.4f) * dt);
		if (step < 1)
			step = 1;
		int toAdd = std::min(step, delta);
		for (int k = 0; k < toAdd && gActivePyramids < pyramidCount; ++k)
		{
			int idx = gActivePyramids;
			pyramids[idx].position.x = frand(-50.0f, 50.0f);
			pyramids[idx].position.z = frand(-50.0f, 50.0f);
			pyramids[idx].position.y = frand(60.0f, 80.0f);
			float maxScale = (pyramids[idx].scale.x > pyramids[idx].scale.y) ? ((pyramids[idx].scale.x > pyramids[idx].scale.z) ? pyramids[idx].scale.x : pyramids[idx].scale.z) : ((pyramids[idx].scale.y > pyramids[idx].scale.z) ? pyramids[idx].scale.y : pyramids[idx].scale.z);
			glm::vec3 half = glm::vec3(maxScale);
			pyramids[idx].boundingBoxMin = pyramids[idx].position - half;
			pyramids[idx].boundingBoxMax = pyramids[idx].position + half;
			gActivePyramids++;
		}
		rebuildBVHForActive();
	}
	else if (delta < 0)
	{
		// Queue deactivations for the highest-index actives; they'll be retired when they exit view (fall below)
		// Faster ramp-down scaled by fall speed
		int step = (int)ceilf((-delta) * (0.8f + cp.fallSpeed * 0.4f) * dt);
		if (step < 1)
			step = 1;
		int wantDeactivate = std::min(step, -delta);
		if (gDeactivateFlags.empty())
			gDeactivateFlags.assign(MAX_PYRAMIDS, 0);
		for (int k = 0; k < wantDeactivate; ++k)
		{
			int idx = gActivePyramids - 1 - (gPendingDeactivations + k);
			if (idx >= 0)
				gDeactivateFlags[idx] = 1;
		}
		gPendingDeactivations += wantDeactivate;
	}
}

static void updateGusts(float dt)
{
	if (!gSettings.gustsEnabled)
	{
		gGustActive = false;
		return;
	}
	gGustTimer += dt;
	if (!gGustActive)
	{
		if (gGustTimer >= gSettings.gustIntervalSeconds)
		{
			gGustActive = true;
			gGustElapsed = 0.0f;
			gGustTimer = 0.0f;
		}
	}
	else
	{
		gGustElapsed += dt;
		if (gGustElapsed >= gSettings.gustDurationSeconds)
		{
			gGustActive = false;
			gGustElapsed = 0.0f;
		}
	}
}

static GLuint loadShadersForSettings(const AppSettings &settings)
{
	bool usingEnhanced = false;
	GLuint program = 0;
	if (settings.shaderType == ShaderType::Phong)
	{
		program = initShaders((char *)"../shaders/phong_simple.vert", (char *)"../shaders/phong_simple.frag");
		if (program != 0)
			usingEnhanced = true;
		if (!usingEnhanced)
		{
			program = initShaders((char *)"shaders/phong_simple.vert", (char *)"shaders/phong_simple.frag");
			if (program != 0)
				usingEnhanced = true;
		}
		if (!usingEnhanced)
		{
			program = 0;
		}
	}
	else if (settings.shaderType == ShaderType::SnowGlow)
	{
		program = initShaders((char *)"../shaders/snow_glow.vert", (char *)"../shaders/snow_glow.frag");
		if (program == 0)
			program = initShaders((char *)"shaders/snow_glow.vert", (char *)"shaders/snow_glow.frag");
	}
	if (program == 0)
	{
		program = initShaders((char *)"../simple.vert", (char *)"../simple.frag");
		if (program == 0)
		{
			program = initShaders((char *)"simple.vert", (char *)"simple.frag");
		}
	}
	return program;
}

void init(void)
{
	DEBUG_SEPARATOR;
	DEBUG_LOG("=== INITIALIZATION START ===");
	DEBUG_LOG_VALUE("Camera Position", "(" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")");
	DEBUG_LOG_VALUE("Camera Front", "(" << cameraFront.x << ", " << cameraFront.y << ", " << cameraFront.z << ")");
	DEBUG_LOG_VALUE("Camera Up", "(" << cameraUp.x << ", " << cameraUp.y << ", " << cameraUp.z << ")");

	// Pyramid geometry (6 vertices as triangle fan)
	const GLfloat pyramidVerts[18] = {
		0.0f, 0.5f, 0.0f,
		-1.0f, -0.5f, 1.0f,
		1.0f, -0.5f, 1.0f,
		1.0f, -0.5f, -1.0f,
		-1.0f, -0.5f, -1.0f,
		-1.0f, -0.5f, 1.0f};
	const GLfloat pyramidCols[18] = {
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f};

	DEBUG_LOG("Loading shaders...");
	shaderprogram = loadShadersForSettings(gSettings);
	if (shaderprogram != 0)
	{
		DEBUG_LOG_VALUE("Shader program loaded", "SUCCESS");
	}
	else
	{
		DEBUG_LOG_VALUE("Shader program loaded", "FAILED");
	}

	if (shaderprogram == 0)
	{
		cout << "Failed to create shader program!" << endl;
		return;
	}

	cout << ((gSettings.shaderType == ShaderType::Phong) ? "Using enhanced Phong lighting shaders" : "Using basic shaders") << endl;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(2, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidVerts), pyramidVerts, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidCols), pyramidCols, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	// Static quad (two triangles) on XZ plane centered at origin
	const GLfloat quad[18] = {
		-1.0f, 0.0f, -1.0f,
		1.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 1.0f,
		-1.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 1.0f,
		-1.0f, 0.0f, 1.0f};
	const GLfloat quadColorStatic[18] = {
		0.12f, 0.16f, 0.22f,
		0.12f, 0.16f, 0.22f,
		0.12f, 0.16f, 0.22f,
		0.12f, 0.16f, 0.22f,
		0.12f, 0.16f, 0.22f,
		0.12f, 0.16f, 0.22f};

	glGenVertexArrays(1, &vaoStatic);
	glBindVertexArray(vaoStatic);
	glGenBuffers(2, vboStatic);
	glBindBuffer(GL_ARRAY_BUFFER, vboStatic[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vboStatic[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadColorStatic), quadColorStatic, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	// Impostor quad (camera-facing), positions in NDC-like local space [-1,1]
	const GLfloat quadImpostor[12] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f};
	const GLfloat quadImpostorColor[12] = {
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f,
		0.95f, 0.95f, 0.98f};

	glGenVertexArrays(1, &vaoImpostor);
	glBindVertexArray(vaoImpostor);
	glGenBuffers(2, vboImpostor);
	glBindBuffer(GL_ARRAY_BUFFER, vboImpostor[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadImpostor), quadImpostor, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vboImpostor[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadImpostorColor), quadImpostorColor, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glEnable(GL_DEPTH_TEST);

	// Build full pool
	DEBUG_LOG("Generating particle pool...");
	generatePyramids(MAX_PYRAMIDS);
	DEBUG_LOG_VALUE("Generated particles", pyramidCount);

	// Prewarm visible subset near camera
	DEBUG_LOG("Prewarming visible particles...");
	{
		int pre = std::min(5000, MAX_PYRAMIDS);
		DEBUG_LOG_VALUE("Prewarm count", pre);
		for (int i = 0; i < pre; ++i)
		{
			pyramids[i].position.x = cameraPos.x + frand(-20.0f, 20.0f);
			pyramids[i].position.z = cameraPos.z + frand(-15.0f, 10.0f);
			pyramids[i].position.y = cameraPos.y + frand(5.0f, 20.0f);
			float s = pyramids[i].scale.x;
			glm::vec3 half = glm::vec3(s);
			pyramids[i].boundingBoxMin = pyramids[i].position - half;
			pyramids[i].boundingBoxMax = pyramids[i].position + half;
		}
		std::cout << "[DEBUG] Prewarm position range: X: " << (cameraPos.x - 20.0f) << " to " << (cameraPos.x + 20.0f)
				  << ", Z: " << (cameraPos.z - 15.0f) << " to " << (cameraPos.z + 10.0f)
				  << ", Y: " << (cameraPos.y + 5.0f) << " to " << (cameraPos.y + 20.0f) << std::endl;
	}
	gDeactivateFlags.assign(MAX_PYRAMIDS, 0);

	int presetIdx = 0;
	if (gSettings.cadenceSelection == CadenceSelection::One)
		presetIdx = 0;
	else if (gSettings.cadenceSelection == CadenceSelection::Two)
		presetIdx = 1;
	else if (gSettings.cadenceSelection == CadenceSelection::Three)
		presetIdx = 2;
	else
		presetIdx = ((int)(gCadenceTimer / std::max(0.001f, gSettings.cadenceCycleSeconds))) % 3;

	DEBUG_LOG_VALUE("Selected cadence preset", presetIdx);
	DEBUG_LOG_VALUE("Preset pyramids", gSettings.cadence[presetIdx].pyramids);

	gTargetActivePyramids = std::max(0, std::min(MAX_PYRAMIDS, gSettings.cadence[presetIdx].pyramids));
	gActivePyramids = std::max(gTargetActivePyramids, 5000);

	DEBUG_LOG_VALUE("Target active pyramids", gTargetActivePyramids);
	DEBUG_LOG_VALUE("Initial active pyramids", gActivePyramids);

	DEBUG_LOG("Building initial BVH...");
	rebuildBVHForActive();
	DEBUG_LOG_VALUE("BVH nodes created", bvhNodeCount);

	DEBUG_LOG("=== INITIALIZATION COMPLETE ===");
	DEBUG_SEPARATOR;
}

void draw(SDL_Window *window)
{
	float currentFrame = SDL_GetTicks() / 1000.0f;
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	gCadenceTimer += deltaTime;
	updateGusts(deltaTime);

	static bool firstFrame = true;
	if (firstFrame)
	{
		DEBUG_SEPARATOR;
		DEBUG_LOG("=== FIRST FRAME RENDERING ===");
		DEBUG_LOG_VALUE("Delta time", deltaTime);
		DEBUG_LOG_VALUE("Cadence timer", gCadenceTimer);
		firstFrame = false;
	}

	glClearColor(gSettings.bgR, gSettings.bgG, gSettings.bgB, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Check if shader program is valid
	if (shaderprogram == 0)
	{
		DEBUG_LOG("ERROR: Invalid shader program!");
		cout << "Invalid shader program!" << endl;
		return;
	}

	// Use the shader program
	glUseProgram(shaderprogram);
	CHECK_GL_ERROR();

	// DEBUG quad will be drawn after projection/view are set

	// DEBUG: Log shader program status
	static bool shaderDebug = true;
	if (shaderDebug)
	{
		std::cout << "[DEBUG] Using shader program ID: " << shaderprogram << std::endl;
		shaderDebug = false;
	}

	applyCadenceIfNeeded(deltaTime);

	// Determine current cadence preset for dynamic behavior
	int presetIdx = 0;
	if (gSettings.cadenceSelection == CadenceSelection::One)
		presetIdx = 0;
	else if (gSettings.cadenceSelection == CadenceSelection::Two)
		presetIdx = 1;
	else if (gSettings.cadenceSelection == CadenceSelection::Three)
		presetIdx = 2;
	else
		presetIdx = ((int)(gCadenceTimer / gSettings.cadenceCycleSeconds)) % 3;
	const CadencePreset &cp = gSettings.cadence[presetIdx];
	float fallMul = gGustActive ? gSettings.gustFallMultiplier : 1.0f;
	float rotMul = gGustActive ? gSettings.gustRotationMultiplier : 1.0f;
	const float speedBoost = gSettings.impostorSpeedMultiplier; // configurable speedup

	// Create perspective projection matrix from settings
	glm::mat4 projection = glm::perspective(glm::radians(gSettings.fovDegrees), 800.0f / 600.0f, gSettings.nearPlane, gSettings.farPlane);

	// Create view matrix for the camera
	glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	// Determine visible pyramids (optionally skip culling)
	std::vector<int> visiblePyramids;
	visiblePyramids.reserve(gActivePyramids);

	static bool firstCulling = true;
	if (firstCulling)
	{
		DEBUG_LOG_VALUE("Active pyramids", gActivePyramids);
		if (gSettings.frustumCullingEnabled)
		{
			DEBUG_LOG_VALUE("Frustum culling enabled", "YES");
		}
		else
		{
			DEBUG_LOG_VALUE("Frustum culling enabled", "NO");
		}
		firstCulling = false;
	}

	if (gSettings.frustumCullingEnabled)
	{
		DEBUG_LOG_VALUE("BVH nodes available", bvhNodeCount);
		Frustum frustum;
		extractFrustumPlanes(projection * view, frustum);
		if (bvhNodeCount > 0)
		{
			traverseBVH(0, frustum, visiblePyramids);
			DEBUG_LOG_VALUE("BVH traversal result", visiblePyramids.size());
		}
		else
		{
			DEBUG_LOG("WARNING: No BVH nodes available for traversal!");
		}
	}
	else
	{
		DEBUG_LOG("Using all active pyramids (no culling)");
		for (int i = 0; i < gActivePyramids; ++i)
			visiblePyramids.push_back(i);
	}

	// pass projection and view matrices as uniforms into shader
	int projectionIndex = glGetUniformLocation(shaderprogram, "projection");
	glUniformMatrix4fv(projectionIndex, 1, GL_FALSE, glm::value_ptr(projection));

	int viewIndex = glGetUniformLocation(shaderprogram, "view");
	glUniformMatrix4fv(viewIndex, 1, GL_FALSE, glm::value_ptr(view));

	// Pass multi-hue blue lights
	int numLightsLoc = glGetUniformLocation(shaderprogram, "numLights");
	int lightPosLoc = glGetUniformLocation(shaderprogram, "lightPos");
	int lightColorLoc = glGetUniformLocation(shaderprogram, "lightColor");
	glUniform3f(glGetUniformLocation(shaderprogram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
	if (numLightsLoc != -1 && lightPosLoc != -1 && lightColorLoc != -1)
	{
		const int n = 3;
		glm::vec3 lp[3] = {
			glm::vec3(gSettings.lightPosX, gSettings.lightPosY, gSettings.lightPosZ),
			glm::vec3(gSettings.lightPosX + 30.0f, gSettings.lightPosY + 15.0f, gSettings.lightPosZ - 20.0f),
			glm::vec3(gSettings.lightPosX - 25.0f, gSettings.lightPosY + 10.0f, gSettings.lightPosZ + 30.0f)};
		glm::vec3 lc[3] = {
			glm::vec3(0.55f, 0.75f, 1.0f), // soft blue
			glm::vec3(0.7f, 0.85f, 1.0f),  // icy blue
			glm::vec3(0.4f, 0.65f, 0.95f)  // deeper cyan-blue
		};
		glUniform1i(numLightsLoc, n);
		glUniform3fv(lightPosLoc, n, glm::value_ptr(lp[0]));
		glUniform3fv(lightColorLoc, n, glm::value_ptr(lc[0]));
	}
	// Optional time and SnowGlow params
	int timeIndex = glGetUniformLocation(shaderprogram, "time");
	if (timeIndex != -1)
	{
		glUniform1f(timeIndex, (float)SDL_GetTicks() / 1000.0f);
		int gi = glGetUniformLocation(shaderprogram, "glowIntensity");
		if (gi != -1)
			glUniform1f(gi, gSettings.snowGlowIntensity);
		int si = glGetUniformLocation(shaderprogram, "sparkleIntensity");
		if (si != -1)
			glUniform1f(si, gSettings.snowSparkleIntensity);
		int st = glGetUniformLocation(shaderprogram, "sparkleThreshold");
		if (st != -1)
			glUniform1f(st, gSettings.snowSparkleThreshold);
		int ns = glGetUniformLocation(shaderprogram, "noiseScale");
		if (ns != -1)
			glUniform1f(ns, gSettings.snowNoiseScale);
		int ts = glGetUniformLocation(shaderprogram, "tintStrength");
		if (ts != -1)
			glUniform1f(ts, gSettings.snowTintStrength);
		int fs = glGetUniformLocation(shaderprogram, "fogStrength");
		if (fs != -1)
			glUniform1f(fs, gSettings.snowFogStrength);
		int bc = glGetUniformLocation(shaderprogram, "bgColor");
		if (bc != -1)
			glUniform3f(bc, gSettings.bgR, gSettings.bgG, gSettings.bgB);
		int rstr = glGetUniformLocation(shaderprogram, "rimStrength");
		if (rstr != -1)
			glUniform1f(rstr, gSettings.snowRimStrength);
		int rpow = glGetUniformLocation(shaderprogram, "rimPower");
		if (rpow != -1)
			glUniform1f(rpow, gSettings.snowRimPower);
		int expo = glGetUniformLocation(shaderprogram, "exposure");
		if (expo != -1)
			glUniform1f(expo, gSettings.snowExposure);
		// Material
		int ru = glGetUniformLocation(shaderprogram, "roughness");
		if (ru != -1)
			glUniform1f(ru, gSettings.snowRoughness);
		int mu = glGetUniformLocation(shaderprogram, "metallic");
		if (mu != -1)
			glUniform1f(mu, gSettings.snowMetallic);
		int su = glGetUniformLocation(shaderprogram, "sss");
		if (su != -1)
			glUniform1f(su, gSettings.snowSSS);
		int au = glGetUniformLocation(shaderprogram, "anisotropy");
		if (au != -1)
			glUniform1f(au, gSettings.snowAnisotropy);
		int ba = glGetUniformLocation(shaderprogram, "baseAlpha");
		if (ba != -1)
			glUniform1f(ba, gSettings.snowBaseAlpha);
		int ef = glGetUniformLocation(shaderprogram, "edgeFade");
		if (ef != -1)
			glUniform1f(ef, gSettings.snowEdgeFade);
		int na = glGetUniformLocation(shaderprogram, "normalAmp");
		if (na != -1)
			glUniform1f(na, gSettings.snowNormalAmplitude);
		int cs = glGetUniformLocation(shaderprogram, "crackScale");
		if (cs != -1)
			glUniform1f(cs, gSettings.snowCrackScale);
		int ci = glGetUniformLocation(shaderprogram, "crackIntensity");
		if (ci != -1)
			glUniform1f(ci, gSettings.snowCrackIntensity);
	}

	// Enable alpha blending for transparent snow
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Draw static geometry first (opaque)
	glBindVertexArray(vaoStatic);
	int modelIndex = glGetUniformLocation(shaderprogram, "model");
	int baIdx = glGetUniformLocation(shaderprogram, "baseAlpha");
	float prevAlpha = gSettings.snowBaseAlpha;
	if (baIdx != -1)
		glUniform1f(baIdx, 1.0f);
	int uDisc = glGetUniformLocation(shaderprogram, "useDisc");
	if (uDisc != -1)
		glUniform1i(uDisc, 0);

	// DEBUG: Draw a bright red quad fixed in front of camera to confirm rendering
	glDisable(GL_DEPTH_TEST);
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, cameraPos + cameraFront * 8.0f + glm::vec3(0.0f, 0.0f, 0.0f));
		M = glm::scale(M, glm::vec3(3.0f, 3.0f, 1.0f));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		// push bright red colors
		glBindBuffer(GL_ARRAY_BUFFER, vboStatic[1]);
		const GLfloat debugColors[18] = {
			1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f,
			1.0f, 0.0f, 0.0f};
		glBufferData(GL_ARRAY_BUFFER, sizeof(debugColors), debugColors, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}
	glEnable(GL_DEPTH_TEST);
	CHECK_GL_ERROR();
	// Main floor (y = -2.0) - visible and close to camera
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, glm::vec3(0.0f, -2.0f, 0.0f));
		M = glm::scale(M, glm::vec3(50.0f, 1.0f, 50.0f));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}

	// Table surface (y = 2.0) - elevated platform for snow accumulation
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, glm::vec3(0.0f, 2.0f, -5.0f));
		M = glm::scale(M, glm::vec3(8.0f, 0.3f, 8.0f));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}

	// Table legs (4 legs)
	const float legHeight = 3.5f;
	const float legRadius = 0.2f;
	const float tableSize = 7.0f;

	// Front left leg
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, glm::vec3(-tableSize * 0.4f, -1.0f + legHeight * 0.5f, -5.0f - tableSize * 0.4f));
		M = glm::scale(M, glm::vec3(legRadius, legHeight, legRadius));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}

	// Front right leg
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, glm::vec3(tableSize * 0.4f, -1.0f + legHeight * 0.5f, -5.0f - tableSize * 0.4f));
		M = glm::scale(M, glm::vec3(legRadius, legHeight, legRadius));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}

	// Back left leg
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, glm::vec3(-tableSize * 0.4f, -1.0f + legHeight * 0.5f, -5.0f + tableSize * 0.4f));
		M = glm::scale(M, glm::vec3(legRadius, legHeight, legRadius));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}

	// Back right leg
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, glm::vec3(tableSize * 0.4f, -1.0f + legHeight * 0.5f, -5.0f + tableSize * 0.4f));
		M = glm::scale(M, glm::vec3(legRadius, legHeight, legRadius));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}

	// Ceiling (rotate 180 around X, y = 25.0) - higher up
	{
		glm::mat4 M(1.0f);
		M = glm::translate(M, glm::vec3(0.0f, 25.0f, 0.0f));
		M = glm::rotate(M, glm::radians(180.0f), glm::vec3(1, 0, 0));
		M = glm::scale(M, glm::vec3(50.0f, 1.0f, 50.0f));
		glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(M));
		glDrawArrays(GL_TRIANGLES, 0, staticVertexCount);
		CHECK_GL_ERROR();
	}
	// Walls removed for visibility
	// (Previously: +X, -X, +Z, -Z quads)
	if (baIdx != -1)
		glUniform1f(baIdx, prevAlpha);

	// DEBUG: Optionally skip particle rendering to test base scene
	static bool skipParticles = false;
	if (!skipParticles)
	{
		// Draw all visible pyramids (as billboarded impostor spheres when SnowGlow is active)
		float time = (float)SDL_GetTicks() / 1000.0f;
		modelIndex = glGetUniformLocation(shaderprogram, "model");
		int uUseDisc = glGetUniformLocation(shaderprogram, "useDisc");
		int uUseBillboard = glGetUniformLocation(shaderprogram, "useBillboard");
		int uBillboardCenter = glGetUniformLocation(shaderprogram, "billboardCenter");
		int uSpriteSize = glGetUniformLocation(shaderprogram, "spriteSize");
		int uLodLevel = glGetUniformLocation(shaderprogram, "lodLevel");

		// Enable billboard impostors with disc masking (spheres)
		if (uUseDisc != -1)
			glUniform1i(uUseDisc, 1);
		if (uUseBillboard != -1)
			glUniform1i(uUseBillboard, 1);
		std::cout << "[DEBUG] Using billboard mode with disc masking for sphere rendering" << std::endl;

		// Debug: Log first few particles
		static bool firstParticleDebug = true;
		if (firstParticleDebug && !visiblePyramids.empty())
		{
			int debugCount = std::min(5, (int)visiblePyramids.size());
			for (int i = 0; i < debugCount; ++i)
			{
				int pyramidIndex = visiblePyramids[i];
				if (pyramidIndex >= 0 && pyramidIndex < gActivePyramids)
				{
					Pyramid &pyramid = pyramids[pyramidIndex];
					std::cout << "[DEBUG] Particle " << i << ": Pos("
							  << pyramid.position.x << ", " << pyramid.position.y << ", " << pyramid.position.z
							  << ") Scale(" << pyramid.scale.x << ") Distance: "
							  << glm::length(pyramid.position - cameraPos) << std::endl;
				}
			}
			firstParticleDebug = false;
		}

		// Render impostor spheres after opaque, disable depth writes
		glDepthMask(GL_FALSE);
		int viewW = 0, viewH = 0;
		SDL_GetWindowSize(window, &viewW, &viewH);
		glBindVertexArray(vaoImpostor);

		// Precompute camera constants for screen-space calculations
		bool cameraChanged = (gLastFOV != gSettings.fovDegrees || gLastWindowWidth != viewW || gLastWindowHeight != viewH);
		if (cameraChanged)
		{
			gLastFOV = gSettings.fovDegrees;
			gLastWindowWidth = viewW;
			gLastWindowHeight = viewH;
			float fovy = glm::radians(gSettings.fovDegrees);
			gPixelsPerUnitY = (float(viewH) * 0.5f) / tanf(fovy * 0.5f);
		}
		int spDrawn = 0;
		int spCulledOffscreen = 0;
		int spCulledTiny = 0;
		int spBudgetCap = 0;
		int drawnThisFrame = 0; // reset each frame
		// DEBUG: Log how many particles we're trying to render
		static bool particleCountDebug = true;
		if (particleCountDebug)
		{
			std::cout << "[DEBUG] About to render " << visiblePyramids.size() << " visible particles" << std::endl;
			particleCountDebug = false;
		}

		for (int pyramidIndex : visiblePyramids)
		{
			static const int kMaxImpostorsPerFrame = 30000; // safety cap
			if (drawnThisFrame >= kMaxImpostorsPerFrame)
			{
				spBudgetCap++;
				break;
			}
			if (pyramidIndex >= 0 && pyramidIndex < gActivePyramids)
			{
				Pyramid &pyramid = pyramids[pyramidIndex];

				// Gust jitter for variety
				float jitter = gGustActive ? (0.8f + 0.4f * ((float)((pyramidIndex * 16807) % 1000) / 1000.0f)) : 1.0f;

				// Fall until floor, table, or short quota, then land for 5s, then retire or respawn in front of camera
				if (!pyramid.landed)
				{
					float dy = cp.fallSpeed * speedBoost * fallMul * jitter * deltaTime;
					pyramid.position.y -= dy;
					pyramid.fallDistanceRemaining -= dy;

					// Check collision with different surfaces
					float particleY = pyramid.position.y;
					float tableTop = 2.3f;	  // Table surface y + half height
					float floorLevel = -2.0f; // Main floor level
					bool hitTable = particleY <= tableTop && pyramid.position.x >= -4.0f && pyramid.position.x <= 4.0f &&
									pyramid.position.z >= -9.0f && pyramid.position.z <= -1.0f;
					bool hitFloor = particleY <= floorLevel;
					bool reachedQuota = pyramid.fallDistanceRemaining <= 0.0f;

					if (hitTable || hitFloor || reachedQuota)
					{
						if (hitTable)
						{
							pyramid.position.y = tableTop; // Land on table
						}
						else
						{
							pyramid.position.y = floorLevel; // Land on floor
						}
						pyramid.landed = true;
						pyramid.landedTimer = 0.0f;
					}
				}
				else
				{
					pyramid.landedTimer += deltaTime;
					if (pyramid.landedTimer >= 5.0f)
					{
						// If pending deactivation and this index is marked, retire it; else respawn in front of camera
						if (gPendingDeactivations > 0 && !gDeactivateFlags.empty() && gDeactivateFlags[pyramidIndex])
						{
							int last = gActivePyramids - 1;
							if (pyramidIndex != last)
							{
								std::swap(pyramids[pyramidIndex], pyramids[last]);
								std::swap(gDeactivateFlags[pyramidIndex], gDeactivateFlags[last]);
							}
							gActivePyramids--;
							gPendingDeactivations--;
							gDeactivateFlags[last] = 0; // clear moved flag
							invalidateBVH();
							continue;
						}
						// Respawn in front of camera, above visible area
						pyramid.position.x = cameraPos.x + frand(-15.0f, 15.0f);
						pyramid.position.z = cameraPos.z + frand(-10.0f, 8.0f);
						pyramid.position.y = cameraPos.y + frand(8.0f, 18.0f);
						pyramid.landed = false;
						pyramid.landedTimer = 0.0f;
						pyramid.fallDistanceRemaining = frand(8.0f, 20.0f);
					}
				}

				// Update AABB vertical
				float maxScale = (pyramid.scale.x > pyramid.scale.y) ? ((pyramid.scale.x > pyramid.scale.z) ? pyramid.scale.x : pyramid.scale.z) : ((pyramid.scale.y > pyramid.scale.z) ? pyramid.scale.y : pyramid.scale.z);
				glm::vec3 halfSize = glm::vec3(maxScale, maxScale, maxScale);
				pyramid.boundingBoxMin = pyramid.position - halfSize;
				pyramid.boundingBoxMax = pyramid.position + halfSize;

				// Early distance-based culling (most efficient)
				float distance = glm::length(pyramid.position - cameraPos);
				bool debugForceDraw = true; // Force draw to diagnose visibility
				if (gSettings.enableDistanceCulling && distance > gSettings.maxRenderDistance)
				{
					spCulledOffscreen++;
					if (!debugForceDraw)
						continue;
				}

				// Optimized screen-space LOD and extra culling
				glm::vec4 clipPos = projection * view * glm::vec4(pyramid.position, 1.0f);
				if (clipPos.w <= 0.0f)
				{
					spCulledOffscreen++;
					if (!debugForceDraw)
						continue;
				}
				float ndcX = clipPos.x / clipPos.w;
				float ndcY = clipPos.y / clipPos.w;
				if (ndcX < -1.5f || ndcX > 1.5f || ndcY < -1.5f || ndcY > 1.5f)
				{
					spCulledOffscreen++;
					if (!debugForceDraw)
						continue;
				}

				// Use precomputed camera constant for screen-space calculations
				float radiusWorld = pyramid.scale.x * 0.5f;
				float screenRadiusPx = gPixelsPerUnitY * (radiusWorld / std::max(0.001f, distance));
				if (gSettings.enableScreenSpaceCulling && screenRadiusPx < gSettings.minScreenPixels)
				{
					spCulledTiny++;
					if (!debugForceDraw)
						continue; // allow drawing for debugging
				}

				// Optimized LOD calculation using configurable thresholds
				float lod = 1.0f;
				if (screenRadiusPx < gSettings.lodMidThreshold)
				{
					lod = 0.5f;
					if (screenRadiusPx < gSettings.lodNearThreshold)
					{
						lod = 0.0f;
					}
				}

				// Set billboard center and sprite size (impostor spheres)
				if (uBillboardCenter != -1)
					glUniform3f(uBillboardCenter, pyramid.position.x, pyramid.position.y, pyramid.position.z);

				// Batch uniforms - only update when values change significantly
				static glm::vec3 lastPos(-99999.0f);
				static float lastSize = -1.0f;
				static float lastLod = -1.0f;

				if (gSettings.enableUniformBatching)
				{
					bool posChanged = glm::distance(lastPos, pyramid.position) > 0.01f;
					bool sizeChanged = abs(lastSize - pyramid.scale.x) > 0.001f;
					bool lodChanged = abs(lastLod - lod) > 0.1f;

					if (posChanged || drawnThisFrame == 0)
					{
						if (uBillboardCenter != -1)
							glUniform3f(uBillboardCenter, pyramid.position.x, pyramid.position.y, pyramid.position.z);
						lastPos = pyramid.position;
					}

					if (sizeChanged || lodChanged || drawnThisFrame == 0)
					{
						float sizeScale = (lod >= 1.0f ? 2.5f : (lod >= 0.5f ? 3.0f : 3.5f)) * gSettings.impostorSizeMultiplier;
						float spriteHalfSize = pyramid.scale.x * 0.5f * sizeScale;
						spriteHalfSize = glm::clamp(spriteHalfSize, gSettings.impostorMinWorldSize, gSettings.impostorMaxWorldSize);
						if (uSpriteSize != -1)
							glUniform1f(uSpriteSize, spriteHalfSize);
						if (uLodLevel != -1)
							glUniform1f(uLodLevel, lod);
						lastSize = pyramid.scale.x;
						lastLod = lod;
					}
				}
				else
				{
					// Update uniforms for every particle if batching is disabled
					if (uBillboardCenter != -1)
						glUniform3f(uBillboardCenter, pyramid.position.x, pyramid.position.y, pyramid.position.z);
					float sizeScale = (lod >= 1.0f ? 2.5f : (lod >= 0.5f ? 3.0f : 3.5f)) * gSettings.impostorSizeMultiplier;
					float spriteHalfSize = pyramid.scale.x * 0.5f * sizeScale;
					spriteHalfSize = glm::clamp(spriteHalfSize, gSettings.impostorMinWorldSize, gSettings.impostorMaxWorldSize);
					if (uSpriteSize != -1)
						glUniform1f(uSpriteSize, spriteHalfSize);
					if (uLodLevel != -1)
						glUniform1f(uLodLevel, lod);
				}

				// Draw impostor sphere (quad fan)
				glBindVertexArray(vaoImpostor);
				glDrawArrays(GL_TRIANGLE_FAN, 0, impostorVertexCount);
				CHECK_GL_ERROR();
				drawnThisFrame++;
				spDrawn++;
			}
		}
		glDepthMask(GL_TRUE);

		// Feed UI overlay with debug counters
		UI_SetDebugStats(gActivePyramids, (int)visiblePyramids.size(), spDrawn, spCulledOffscreen, spCulledTiny, spBudgetCap);
	}

	// Draw all visible pyramids (as billboarded impostor spheres when SnowGlow is active)
	float time = (float)SDL_GetTicks() / 1000.0f;
	modelIndex = glGetUniformLocation(shaderprogram, "model");
	int uUseDisc = glGetUniformLocation(shaderprogram, "useDisc");
	int uUseBillboard = glGetUniformLocation(shaderprogram, "useBillboard");
	int uBillboardCenter = glGetUniformLocation(shaderprogram, "billboardCenter");
	int uSpriteSize = glGetUniformLocation(shaderprogram, "spriteSize");
	int uLodLevel = glGetUniformLocation(shaderprogram, "lodLevel");

	// Enable proper sphere rendering with billboards and disc masking
	if (uUseDisc != -1)
		glUniform1i(uUseDisc, 1); // Enable disc masking for proper sphere appearance
	if (uUseBillboard != -1)
		glUniform1i(uUseBillboard, 1); // Enable billboard mode for camera-facing spheres
	std::cout << "[DEBUG] Using billboard mode with disc masking for sphere rendering" << std::endl;

	// Debug: Log first few particles
	static bool firstParticleDebug = true;
	if (firstParticleDebug && !visiblePyramids.empty())
	{
		int debugCount = std::min(5, (int)visiblePyramids.size());
		for (int i = 0; i < debugCount; ++i)
		{
			int pyramidIndex = visiblePyramids[i];
			if (pyramidIndex >= 0 && pyramidIndex < gActivePyramids)
			{
				Pyramid &pyramid = pyramids[pyramidIndex];
				std::cout << "[DEBUG] Particle " << i << ": Pos("
						  << pyramid.position.x << ", " << pyramid.position.y << ", " << pyramid.position.z
						  << ") Scale(" << pyramid.scale.x << ") Distance: "
						  << glm::length(pyramid.position - cameraPos) << std::endl;
			}
		}
		std::cout << "[DEBUG] Shader uniforms: useBillboard=" << (uUseBillboard != -1 ? "FOUND" : "NOT FOUND")
				  << " billboardCenter=" << (uBillboardCenter != -1 ? "FOUND" : "NOT FOUND")
				  << " spriteSize=" << (uSpriteSize != -1 ? "FOUND" : "NOT FOUND") << std::endl;
		firstParticleDebug = false;
	}

	static int frameCount = 0;
	frameCount++;

	// Performance tracking
	totalFrames++;
	totalPyramidsRendered += (int)visiblePyramids.size();

	// Only print stats every 60 frames to avoid spam
	if (frameCount % 60 == 0)
	{
		float avgPyramidsPerFrame = (float)totalPyramidsRendered / totalFrames;
		cout << "Rendering " << visiblePyramids.size() << " / " << gActivePyramids << " snow pyramids (culled "
			 << (gActivePyramids - (int)visiblePyramids.size()) << ") | Avg: " << avgPyramidsPerFrame << " pyramids/frame" << endl;
	}

	// Render impostors after opaque, disable depth writes to reduce harsh sorting artifacts
	glDepthMask(GL_FALSE);
	int viewW = 0, viewH = 0;
	SDL_GetWindowSize(window, &viewW, &viewH);
	glBindVertexArray(vaoImpostor);

	// Precompute camera constants for screen-space calculations
	bool cameraChanged = (gLastFOV != gSettings.fovDegrees || gLastWindowWidth != viewW || gLastWindowHeight != viewH);
	if (cameraChanged)
	{
		gLastFOV = gSettings.fovDegrees;
		gLastWindowWidth = viewW;
		gLastWindowHeight = viewH;
		float fovy = glm::radians(gSettings.fovDegrees);
		gPixelsPerUnitY = (float(viewH) * 0.5f) / tanf(fovy * 0.5f);
	}
	int spDrawn = 0;
	int spCulledOffscreen = 0;
	int spCulledTiny = 0;
	int spBudgetCap = 0;
	int drawnThisFrame = 0; // reset each frame
	// DEBUG: Log how many particles we're trying to render
	static bool particleCountDebug = true;
	if (particleCountDebug)
	{
		std::cout << "[DEBUG] About to render " << visiblePyramids.size() << " visible particles" << std::endl;
		particleCountDebug = false;
	}

	for (int pyramidIndex : visiblePyramids)
	{
		static const int kMaxImpostorsPerFrame = 30000; // safety cap
		if (drawnThisFrame >= kMaxImpostorsPerFrame)
		{
			spBudgetCap++;
			break;
		}
		if (pyramidIndex >= 0 && pyramidIndex < gActivePyramids)
		{
			Pyramid &pyramid = pyramids[pyramidIndex];

			// Gust jitter for variety
			float jitter = gGustActive ? (0.8f + 0.4f * ((float)((pyramidIndex * 16807) % 1000) / 1000.0f)) : 1.0f;

			// Fall until floor, table, or short quota, then land for 5s, then retire or respawn in front of camera
			if (!pyramid.landed)
			{
				float dy = cp.fallSpeed * speedBoost * fallMul * jitter * deltaTime;
				pyramid.position.y -= dy;
				pyramid.fallDistanceRemaining -= dy;

				// Check collision with different surfaces
				float particleY = pyramid.position.y;
				float tableTop = 2.3f;	  // Table surface y + half height
				float floorLevel = -2.0f; // Main floor level
				bool hitTable = particleY <= tableTop && pyramid.position.x >= -4.0f && pyramid.position.x <= 4.0f &&
								pyramid.position.z >= -9.0f && pyramid.position.z <= -1.0f;
				bool hitFloor = particleY <= floorLevel;
				bool reachedQuota = pyramid.fallDistanceRemaining <= 0.0f;

				if (hitTable || hitFloor || reachedQuota)
				{
					if (hitTable)
					{
						pyramid.position.y = tableTop; // Land on table
						static int tableLands = 0;
						if (++tableLands % 50 == 0)
						{
							std::cout << "[DEBUG] Snow particle landed on table! Total: " << tableLands << std::endl;
						}
					}
					else
					{
						pyramid.position.y = floorLevel; // Land on floor
						static int floorLands = 0;
						if (++floorLands % 100 == 0)
						{
							std::cout << "[DEBUG] Snow particle landed on floor! Total: " << floorLands << std::endl;
						}
					}
					pyramid.landed = true;
					pyramid.landedTimer = 0.0f;
				}
			}
			else
			{
				pyramid.landedTimer += deltaTime;
				if (pyramid.landedTimer >= 5.0f)
				{
					// If pending deactivation and this index is marked, retire it; else respawn in front of camera
					if (gPendingDeactivations > 0 && !gDeactivateFlags.empty() && gDeactivateFlags[pyramidIndex])
					{
						int last = gActivePyramids - 1;
						if (pyramidIndex != last)
						{
							std::swap(pyramids[pyramidIndex], pyramids[last]);
							std::swap(gDeactivateFlags[pyramidIndex], gDeactivateFlags[last]);
						}
						gActivePyramids--;
						gPendingDeactivations--;
						gDeactivateFlags[last] = 0; // clear moved flag
						invalidateBVH();
						continue;
					}
					// Respawn in front of camera, above visible area
					pyramid.position.x = cameraPos.x + frand(-15.0f, 15.0f);
					pyramid.position.z = cameraPos.z + frand(-10.0f, 8.0f);
					pyramid.position.y = cameraPos.y + frand(8.0f, 18.0f);
					pyramid.landed = false;
					pyramid.landedTimer = 0.0f;
					pyramid.fallDistanceRemaining = frand(8.0f, 20.0f);
				}
			}

			// Update AABB vertical
			float maxScale = (pyramid.scale.x > pyramid.scale.y) ? ((pyramid.scale.x > pyramid.scale.z) ? pyramid.scale.x : pyramid.scale.z) : ((pyramid.scale.y > pyramid.scale.z) ? pyramid.scale.y : pyramid.scale.z);
			glm::vec3 halfSize = glm::vec3(maxScale, maxScale, maxScale);
			pyramid.boundingBoxMin = pyramid.position - halfSize;
			pyramid.boundingBoxMax = pyramid.position + halfSize;

			// Early distance-based culling (most efficient)
			float distance = glm::length(pyramid.position - cameraPos);
			if (gSettings.enableDistanceCulling && distance > gSettings.maxRenderDistance)
			{
				spCulledOffscreen++;
				continue;
			}

			// Optimized screen-space LOD and extra culling
			glm::vec4 clipPos = projection * view * glm::vec4(pyramid.position, 1.0f);
			if (clipPos.w <= 0.0f)
			{
				spCulledOffscreen++;
				continue;
			}
			float ndcX = clipPos.x / clipPos.w;
			float ndcY = clipPos.y / clipPos.w;
			if (ndcX < -1.2f || ndcX > 1.2f || ndcY < -1.2f || ndcY > 1.2f)
			{
				spCulledOffscreen++;
				continue;
			}

			// Use precomputed camera constant for screen-space calculations
			float radiusWorld = pyramid.scale.x * 0.5f;
			float screenRadiusPx = gPixelsPerUnitY * (radiusWorld / std::max(0.001f, distance));
			if (gSettings.enableScreenSpaceCulling && screenRadiusPx < gSettings.minScreenPixels)
			{
				spCulledTiny++;
				continue; // too small to matter
			}

			// Optimized LOD calculation using configurable thresholds
			float lod = 1.0f;
			if (screenRadiusPx < gSettings.lodMidThreshold)
			{
				lod = 0.5f;
				if (screenRadiusPx < gSettings.lodNearThreshold)
				{
					lod = 0.0f;
				}
			}

			// Set model matrix for this particle's position and scale
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, pyramid.position);
			model = glm::scale(model, pyramid.scale);
			glUniformMatrix4fv(modelIndex, 1, GL_FALSE, glm::value_ptr(model));

			// Batch uniforms - only update when values change significantly
			static glm::vec3 lastPos(-99999.0f);
			static float lastSize = -1.0f;
			static float lastLod = -1.0f;

			if (gSettings.enableUniformBatching)
			{
				bool posChanged = glm::distance(lastPos, pyramid.position) > 0.01f;
				bool sizeChanged = abs(lastSize - pyramid.scale.x) > 0.001f;
				bool lodChanged = abs(lastLod - lod) > 0.1f;

				if (posChanged || drawnThisFrame == 0)
				{
					if (uBillboardCenter != -1)
						glUniform3f(uBillboardCenter, pyramid.position.x, pyramid.position.y, pyramid.position.z);
					lastPos = pyramid.position;
				}

				if (sizeChanged || lodChanged || drawnThisFrame == 0)
				{
					float sizeScale = (lod >= 1.0f ? 2.5f : (lod >= 0.5f ? 3.0f : 3.5f)) * gSettings.impostorSizeMultiplier;
					float spriteHalfSize = glm::max(0.1f, pyramid.scale.x * 0.5f * sizeScale);
					if (uSpriteSize != -1)
						glUniform1f(uSpriteSize, spriteHalfSize);
					if (uLodLevel != -1)
						glUniform1f(uLodLevel, lod);
					lastSize = pyramid.scale.x;
					lastLod = lod;
				}
			}
			else
			{
				// Update uniforms for every particle if batching is disabled
				if (uBillboardCenter != -1)
					glUniform3f(uBillboardCenter, pyramid.position.x, pyramid.position.y, pyramid.position.z);
				float sizeScale = (lod >= 1.0f ? 2.5f : (lod >= 0.5f ? 3.0f : 3.5f)) * gSettings.impostorSizeMultiplier;
				float spriteHalfSize = pyramid.scale.x * 0.5f * sizeScale;
				spriteHalfSize = glm::clamp(spriteHalfSize, gSettings.impostorMinWorldSize, gSettings.impostorMaxWorldSize);
				if (uSpriteSize != -1)
					glUniform1f(uSpriteSize, spriteHalfSize);
				if (uLodLevel != -1)
					glUniform1f(uLodLevel, lod);
			}

			// Draw the billboarded quad
			glDrawArrays(GL_TRIANGLE_FAN, 0, impostorVertexCount);
			CHECK_GL_ERROR();
			drawnThisFrame++;
			spDrawn++;
		}
	}
	glDepthMask(GL_TRUE);

	// Feed UI overlay with debug counters
	UI_SetDebugStats(gActivePyramids, (int)visiblePyramids.size(), spDrawn, spCulledOffscreen, spCulledTiny, spBudgetCap);

	static bool firstRender = true;
	if (firstRender)
	{
		DEBUG_LOG("=== RENDERING STATS ===");
		DEBUG_LOG_VALUE("Total visible pyramids", visiblePyramids.size());
		DEBUG_LOG_VALUE("Pyramids drawn", spDrawn);
		DEBUG_LOG_VALUE("Culled offscreen", spCulledOffscreen);
		DEBUG_LOG_VALUE("Culled tiny", spCulledTiny);
		DEBUG_LOG_VALUE("Budget cap hits", spBudgetCap);
		DEBUG_SEPARATOR;
		firstRender = false;
	}

	if (!gUIState.open && gSettings.debugOverlayEnabled)
	{
		int wmini = 0, hmini = 0;
		SDL_GetWindowSize(window, &wmini, &hmini);
		UI_DrawCountersMini(wmini, hmini, gActivePyramids, (int)visiblePyramids.size(), spDrawn, spCulledOffscreen, spCulledTiny, spBudgetCap);
	}

	int w = 0, h = 0;
	SDL_GetWindowSize(window, &w, &h);
	UI_BeginFrame();
	UI_Draw(gUIState, gSettings, w, h);

	SDL_GL_SwapWindow(window); // swap buffers
}

void cleanup(void)
{
	glUseProgram(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	// could also detach shaders
	glDeleteProgram(shaderprogram);
	glDeleteBuffers(2, vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(2, vboStatic);
	glDeleteVertexArrays(1, &vaoStatic);
	glDeleteBuffers(2, vboImpostor);
	glDeleteVertexArrays(1, &vaoImpostor);
}

int main(int argc, char *argv[])
{
	DEBUG_SEPARATOR;
	DEBUG_LOG("=== APPLICATION START ===");

	// INICIALIZACION
	DEBUG_LOG("Initializing SDL...");
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
	{
		DEBUG_LOG("ERROR: Failed to initialize SDL!");
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return 1;
	}
	DEBUG_LOG("SDL initialized successfully");

	DEBUG_LOG("Creating window...");
	SDL_Window *window = NULL;
	SDL_GLContext gl_context;

	window = SDL_CreateWindow("Winter Snow Scene", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							  800, 600, SDL_WINDOW_OPENGL);

	if (!window)
	{
		DEBUG_LOG("ERROR: Failed to create window!");
		return 1;
	}
	DEBUG_LOG("Window created successfully");

	DEBUG_LOG("Creating OpenGL context...");
	gl_context = SDL_GL_CreateContext(window);
	if (!gl_context)
	{
		DEBUG_LOG("ERROR: Failed to create OpenGL context!");
		return 1;
	}
	DEBUG_LOG("OpenGL context created successfully");

	// disable limit of 60fps
	DEBUG_LOG("Disabling VSync...");
	SDL_GL_SetSwapInterval(0);

	// Check OpenGL properties
	DEBUG_LOG("Loading OpenGL functions...");
	gladLoadGLLoader(SDL_GL_GetProcAddress);
	DEBUG_LOG("OpenGL functions loaded");

	DEBUG_LOG("OpenGL Info:");
	DEBUG_LOG_VALUE("Vendor", glGetString(GL_VENDOR));
	DEBUG_LOG_VALUE("Renderer", glGetString(GL_RENDERER));
	DEBUG_LOG_VALUE("Version", glGetString(GL_VERSION));

	DEBUG_LOG("Initializing scene...");
	init();

	DEBUG_LOG("Initializing UI...");
	UI_Initialize(window);

	DEBUG_LOG("=== APPLICATION READY ===");
	DEBUG_SEPARATOR;

	// Print instructions
	cout << "\n=== WINTER SNOW SCENE ===" << endl;
	cout << "❄️ RENDERING " << MAX_PYRAMIDS << " SNOW PYRAMIDS WITH BLUE WINTER LIGHTING! ❄️" << endl;
	cout << "Controls:" << endl;
	cout << "  WASD - Move camera horizontally" << endl;
	cout << "  SPACE/SHIFT - Move camera vertically" << endl;
	cout << "  Mouse - Look around (after clicking in window)" << endl;
	cout << "  Left Click - Capture mouse" << endl;
	cout << "  ESC - Release mouse / Exit" << endl;
	cout << "\nWatch the console for culling stats!" << endl;
	cout << "Click inside the window to start!" << endl;

	bool running = true; // set running to true
	SDL_Event sdlEvent;	 // variable to detect SDL events

	while (running) // the event loop
	{
		while (SDL_PollEvent(&sdlEvent))
		{
			bool consumed = false;
			bool needsRegen = false;
			bool needsShaderReload = false;
			int winW = 0, winH = 0;
			SDL_GetWindowSize(window, &winW, &winH);
			consumed = UI_HandleEvent(sdlEvent, gUIState, gSettings, winW, winH, needsRegen, needsShaderReload);
			if (needsRegen)
			{
				generatePyramids(MAX_PYRAMIDS);			 // Regenerate full pool
				gActivePyramids = gTargetActivePyramids; // Reset active to target
				rebuildBVHForActive();
			}
			if (needsShaderReload)
			{
				if (shaderprogram)
				{
					glUseProgram(0);
					glDeleteProgram(shaderprogram);
				}
				shaderprogram = loadShadersForSettings(gSettings);
			}
			if (consumed)
				continue;
			if (sdlEvent.type == SDL_QUIT)
				running = false;
			else if (sdlEvent.type == SDL_MOUSEBUTTONDOWN && sdlEvent.button.button == SDL_BUTTON_LEFT)
			{
				// Capture mouse when left click inside window
				if (!mouseCaptured && !UI_IsOpen(gUIState))
				{
					SDL_SetRelativeMouseMode(SDL_TRUE);
					mouseCaptured = true;
					firstMouse = true;
					cout << "Mouse captured - Press ESC to release" << endl;
				}
			}
			else if (sdlEvent.type == SDL_MOUSEMOTION && mouseCaptured && !UI_IsOpen(gUIState))
			{
				if (firstMouse)
				{
					lastX = 0;
					lastY = 0;
					firstMouse = false;
				}

				float xoffset = (float)sdlEvent.motion.xrel;
				float yoffset = -(float)sdlEvent.motion.yrel; // Inverted Y

				processMouseMovement(xoffset, yoffset);
			}
			else if (sdlEvent.type == SDL_KEYDOWN)
			{
				if (sdlEvent.key.keysym.sym == SDLK_ESCAPE)
				{
					if (mouseCaptured)
					{
						// Release mouse capture
						SDL_SetRelativeMouseMode(SDL_FALSE);
						mouseCaptured = false;
						cout << "Mouse released - Click inside window to capture again" << endl;
					}
					else
					{
						// Exit application
						running = false;
					}
				}
			}
			// Apply vsync if toggled this frame
			SDL_GL_SetSwapInterval(gSettings.vsyncEnabled ? 1 : 0);
		}

		if (!UI_IsOpen(gUIState))
			processKeyboard(); // handle keyboard input
		draw(window);		   // call the draw function
	}

	cleanup();
	UI_Shutdown();

	// FIN LOOP PRINCIPAL
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
