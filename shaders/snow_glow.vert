#version 400 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform int useBillboard;      // 1 for impostor billboards
uniform vec3 billboardCenter;  // world-space center of the sprite
uniform float spriteSize;      // world-space half-size of the sprite
uniform float time;            // animated wind sway
uniform float windStrength;    // 0..1 wind amplitude
uniform float windFrequency;   // oscillation speed
uniform vec3 windDir;          // world-space wind direction (normalized)

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Color;

out vec3 vColor;
out vec3 vWorldPos;
out vec2 vLocal; // normalized quad coords [-1,1] (for impostor disc)

void main(void) {
    if (useBillboard == 1) {
        // Extract camera right/up from view matrix rows (GLSL matrices are column-major)
        vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
        vec3 camUp    = vec3(view[0][1], view[1][1], view[2][1]);

        // Wind sway offset based on position and time
        float w = sin(dot(billboardCenter.xz, vec2(0.05, 0.07)) + time * windFrequency)
                + sin(billboardCenter.y * 0.1 + time * windFrequency * 1.3) * 0.5;
        float heightFactor = clamp(billboardCenter.y / 60.0, 0.0, 1.0);
        vec3 sway = normalize(windDir) * w * windStrength * (0.3 + 0.7 * heightFactor);

        // Build world position from (center + sway) + camera-facing quad
        vec3 centerWS = billboardCenter + sway;
        vec3 worldPos = centerWS + (camRight * in_Position.x + camUp * in_Position.y) * spriteSize;
        vWorldPos = worldPos;
        vColor = in_Color;
        vLocal = in_Position.xy; // already provided in [-1,1]
        gl_Position = projection * view * vec4(worldPos, 1.0);
    } else {
        vWorldPos = vec3(model * vec4(in_Position, 1.0));
        vColor = in_Color;
        vLocal = vec2(0.0);
        gl_Position = projection * view * vec4(vWorldPos, 1.0);
    }
}


