#version 450 core

// Simple line fragment shader
// Outputs interpolated vertex color with optional alpha

in vec3 vColor;

out vec4 fragColor;

uniform float uAlpha;  // Overall transparency (default 1.0)

void main() {
    fragColor = vec4(vColor, uAlpha);
}
