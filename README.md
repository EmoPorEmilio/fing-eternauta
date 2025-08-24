# OpenGL Advanced Graphics Engine

A modern OpenGL-based graphics engine with advanced rendering capabilities,
featuring real-time lighting, camera controls, and CUDA/OptiX integration
potential.

## Features

### Current Features

- ✅ Modern OpenGL 4.0+ with programmable pipeline
- ✅ SDL2 window management and input handling
- ✅ GLM mathematics library integration
- ✅ Basic vertex and fragment shaders
- ✅ Rotating 3D geometry (pyramid)

### Enhanced Features (Proposed)

- 🔄 **Advanced Lighting**: Phong lighting model with ambient, diffuse, and
  specular components
- 🔄 **Interactive Camera**: First-person camera with mouse and keyboard
  controls
- 🔄 **Mesh System**: Flexible geometry management with vertex attributes
- 🔄 **Texture Support**: Material and texture mapping capabilities
- 🔄 **Scene Management**: Multiple object rendering and scene organization
- 🔄 **CUDA/OptiX Integration**: Real-time ray tracing and GPU compute features

## Project Structure

```
opengl-avanzado/
├── main.cpp                 # Original basic implementation
├── main_enhanced.cpp        # Enhanced version with advanced features
├── Camera.h                 # Camera class for view control
├── Mesh.h                   # Mesh class for geometry management
├── shaders/
│   ├── simple.vert         # Basic vertex shader
│   ├── simple.frag         # Basic fragment shader
│   ├── phong.vert          # Phong lighting vertex shader
│   └── phong.frag          # Phong lighting fragment shader
├── libraries/              # Third-party libraries
│   ├── SDL2-2.0.12/       # SDL2 for window management
│   ├── glm/               # GLM mathematics library
│   ├── glad/              # OpenGL loader
│   └── Freeimage/         # Image loading library
├── OpenGL-basico/         # Visual Studio project files
└── CUDA_Integration.md    # CUDA/OptiX integration proposal
```

## Building the Project

### Prerequisites

- Visual Studio 2019 or later
- Windows 10/11
- NVIDIA GPU (for CUDA features)
- CUDA Toolkit 10.2+ (optional)
- OptiX SDK 7.0+ (optional)

### Build Instructions

1. Open `OpenGL-avanzado.sln` in Visual Studio
2. Select your target platform (x64 recommended)
3. Build the solution (Ctrl+Shift+B)
4. Run the application (F5)

### Dependencies

The project includes all necessary libraries:

- **SDL2**: Window management and input handling
- **GLAD**: OpenGL function loading
- **GLM**: Mathematics library
- **FreeImage**: Image loading and processing

## Usage

### Basic Version (main.cpp)

- Displays a rotating colored pyramid
- No user interaction
- Demonstrates basic OpenGL setup

### Enhanced Version (main_enhanced.cpp)

- **WASD**: Move camera forward/backward/left/right
- **Mouse**: Look around (first-person camera)
- **Mouse Wheel**: Zoom in/out
- **ESC**: Exit application

## Development Roadmap

### Phase 1: Core Improvements ✅

- [x] Enhanced shader system
- [x] Camera controls
- [x] Mesh management
- [x] Lighting implementation

### Phase 2: Advanced Features 🔄

- [ ] Texture loading and mapping
- [ ] Multiple object rendering
- [ ] Scene graph implementation
- [ ] Material system

### Phase 3: CUDA/OptiX Integration 📋

- [ ] Real-time ray tracing
- [ ] GPU-accelerated physics
- [ ] Procedural generation
- [ ] Advanced post-processing

### Phase 4: Optimization 🚀

- [ ] Multi-threading
- [ ] Memory optimization
- [ ] Performance profiling
- [ ] VR/AR support

## Technical Details

### OpenGL Version

- **Target**: OpenGL 4.0+
- **Features**: Programmable pipeline, vertex/fragment shaders
- **Extensions**: GLAD for function loading

### Shader System

- **Vertex Shaders**: Transform vertices, pass data to fragment shaders
- **Fragment Shaders**: Calculate pixel colors with lighting
- **Uniforms**: Pass transformation matrices and lighting parameters

### Camera System

- **Type**: First-person camera
- **Controls**: Mouse for rotation, keyboard for movement
- **Features**: Smooth movement, pitch/yaw constraints

### Lighting Model

- **Ambient**: Base illumination level
- **Diffuse**: Directional lighting based on surface normals
- **Specular**: Reflective highlights
- **Parameters**: Configurable strength and shininess

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is for educational purposes. Feel free to use and modify as needed.

## Acknowledgments

- **SDL2**: Simple DirectMedia Layer for cross-platform development
- **GLM**: OpenGL Mathematics library
- **GLAD**: OpenGL function loading
- **FreeImage**: Image loading library
- **NVIDIA**: CUDA and OptiX technologies
