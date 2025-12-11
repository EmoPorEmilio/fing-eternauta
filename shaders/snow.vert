#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aInstanceData;  // xyz = initial position in sphere, w = seed

uniform mat4 uView;
uniform mat4 uProjection;
uniform vec3 uPlayerPos;
uniform float uTime;
uniform float uSphereRadius;
uniform float uFallSpeed;
uniform float uParticleSize;
uniform float uWindStrength;
uniform float uWindAngle;  // Wind direction in radians (synced with 2D overlay)

out vec2 vUV;
flat out float vSeed;

void main() {
    vec3 basePos = aInstanceData.xyz;
    float seed = aInstanceData.w;

    // Animation: fall down over time, using seed to offset each particle's phase
    float cycleTime = 2.0 * uSphereRadius / uFallSpeed;
    float t = mod(uTime + seed * cycleTime, cycleTime);
    float fallOffset = t * uFallSpeed;

    vec3 animPos = basePos;
    animPos.y -= fallOffset;

    // Horizontal drift in wind direction (synced with 2D overlay)
    vec2 windDir = vec2(cos(uWindAngle), sin(uWindAngle));
    float driftAmount = sin(uTime * 0.5 + seed * 6.28) * uWindStrength;
    animPos.x += windDir.x * driftAmount;
    animPos.z += windDir.y * driftAmount;

    // Wrap: when particle falls below sphere bottom, teleport to top
    if (animPos.y < -uSphereRadius) {
        animPos.y += 2.0 * uSphereRadius;
    }

    // World position = player position + local sphere position
    vec3 worldPos = uPlayerPos + animPos;

    // Billboard: extract camera right/up from view matrix
    vec3 camRight = vec3(uView[0][0], uView[1][0], uView[2][0]);
    vec3 camUp = vec3(uView[0][1], uView[1][1], uView[2][1]);

    // Offset quad vertices to form billboard
    worldPos += camRight * aPos.x * uParticleSize + camUp * aPos.y * uParticleSize;

    vUV = aPos * 0.5 + 0.5;
    vSeed = seed;
    gl_Position = uProjection * uView * vec4(worldPos, 1.0);
}
