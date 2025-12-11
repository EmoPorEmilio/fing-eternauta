#version 450 core
// Fullscreen quad vertex shader for post-processing

layout (location = 0) in vec2 aPos;

out vec2 vTexCoord;

void main() {
    vTexCoord = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
