#version 450 core

// Simple line shader for debug axes, gizmos, and wireframes
// Supports per-vertex coloring

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 uMVP;

void main() {
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
