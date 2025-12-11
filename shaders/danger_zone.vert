#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uMonsterPos;
uniform float uRadius;

out vec2 vLocalPos;

void main() {
    // Position vertex in world space around monster
    vec3 worldPos = aPos * uRadius + uMonsterPos;
    worldPos.y = 0.01;  // Slightly above ground to avoid z-fighting

    vLocalPos = aPos.xz;  // Pass local position for circle calculation

    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
