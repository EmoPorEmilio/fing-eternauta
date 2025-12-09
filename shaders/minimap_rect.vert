#version 450 core
layout (location = 0) in vec2 aPos;

uniform mat4 uProjection;
uniform vec2 uRectCenter;
uniform vec2 uRectHalfSize;
uniform float uRotation;

out vec2 vScreenPos;

void main()
{
    // Transform from unit quad (0-1) to centered (-1 to 1)
    vec2 centered = aPos * 2.0 - 1.0;

    // Scale by half size
    vec2 scaled = centered * uRectHalfSize;

    // Rotate around center
    float cosR = cos(uRotation);
    float sinR = sin(uRotation);
    vec2 rotated;
    rotated.x = scaled.x * cosR - scaled.y * sinR;
    rotated.y = scaled.x * sinR + scaled.y * cosR;

    // Translate to rect center
    vec2 worldPos = uRectCenter + rotated;

    vScreenPos = worldPos;
    gl_Position = uProjection * vec4(worldPos, 0.0, 1.0);
}
