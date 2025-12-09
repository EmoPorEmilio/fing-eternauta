#version 450 core

layout(location = 0) in vec2 aPos;

out vec2 vUV;

uniform mat4 uProjection;
uniform vec2 uCenter;
uniform float uRadius;

void main() {
    // aPos is in [-1, 1] range, scale by radius and offset by center
    vec2 pos = aPos * uRadius + uCenter;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
    vUV = aPos;  // Pass normalized position for circle calculation
}
