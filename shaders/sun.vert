#version 450 core
layout (location = 0) in vec2 aPos;  // Quad vertices (-1 to 1)

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uSunWorldPos;  // Sun position far in sky
uniform float uSize;         // Billboard size

out vec2 vUV;

void main() {
    // Extract camera right/up vectors from view matrix for billboarding
    vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 camUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

    // Billboard offset - quad always faces camera
    vec3 worldPos = uSunWorldPos + camRight * aPos.x * uSize + camUp * aPos.y * uSize;

    vUV = aPos * 0.5 + 0.5;  // Convert from -1,1 to 0,1 range
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
