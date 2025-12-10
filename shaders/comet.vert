#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
// Instanced: position (xyz) + timeOffset (w)
layout (location = 3) in vec4 aInstanceData;

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTime;
uniform vec3 uCameraPos;
uniform float uFallSpeed;      // How fast comets fall
uniform float uCycleTime;      // Time for one complete fall cycle
uniform float uFallDistance;   // How far they travel in one cycle
uniform vec3 uFallDirection;   // Normalized direction of fall
uniform float uScale;          // Comet scale
uniform int uDebugMode;        // 1 = show stationary comet for orientation testing
uniform float uTrailStretch;   // How much to stretch along velocity for motion blur trail

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vFragPos;
out vec3 vLocalPos;  // For debug coloring
out float vTrailFade; // 0 at head, 1 at tail - for fading the trail
flat out vec3 vCometCenter; // World position of comet center (flat = no interpolation)
flat out float vTrailLength; // Total trail length for normalization (flat = no interpolation)

// Build rotation matrix to align comet's -Z axis with fall direction
mat3 buildRotationMatrix(vec3 targetDir) {
    // Comet's forward is -Z, so we want -Z to point along targetDir
    // That means +Z should point opposite to targetDir
    vec3 newZ = -targetDir;  // +Z points opposite to fall direction

    // Pick an up vector that isn't parallel to newZ
    vec3 up = abs(newZ.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);

    vec3 newX = normalize(cross(up, newZ));
    vec3 newY = cross(newZ, newX);

    return mat3(newX, newY, newZ);
}

void main()
{
    vec3 startPos = aInstanceData.xyz;
    float timeOffset = aInstanceData.w;

    vec3 worldPos;

    if (uDebugMode == 1) {
        if (gl_InstanceID == 0) {
            // Debug: place comet at FIXED world position (not relative to camera)
            // This allows you to walk up to it and inspect the orientation
            vec3 debugPos = vec3(0.0, 3.0, 0.0);  // At origin, 3 units up
            vec3 scaledPos = aPos * 2.0;  // Fixed size for debug
            worldPos = debugPos + scaledPos;
        } else {
            // Hide other instances in debug mode
            worldPos = vec3(0.0, -10000.0, 0.0);
        }
        vTrailFade = 0.0;
        vCometCenter = worldPos;
        vTrailLength = 1.0;
    } else {
        // Normal mode: animate falling with rotation toward fall direction
        // Add per-instance direction variation so comets don't all fall to the same point
        float instanceSeed = timeOffset * 12.9898;  // Use timeOffset as seed
        float dirVarX = sin(instanceSeed) * 0.15;   // +/- 0.15 variation
        float dirVarZ = cos(instanceSeed * 1.5) * 0.15;
        vec3 instanceFallDir = normalize(uFallDirection + vec3(dirVarX, 0.0, dirVarZ));

        float t = mod(uTime * uFallSpeed + timeOffset, uCycleTime) / uCycleTime;
        vec3 cometPos = startPos + instanceFallDir * t * uFallDistance;
        vec3 worldCometPos = uCameraPos + cometPos;

        // Rotate comet to face its own fall direction (nose pointing where it's going)
        mat3 rotation = buildRotationMatrix(instanceFallDir);
        vec3 rotatedPos = rotation * aPos;

        // Motion blur trail: stretch vertices along the fall direction (in local rotated space)
        // Vertices with positive Z (tail) get stretched backward more
        // The comet's local +Z points opposite to travel direction (tail)
        float tailAmount = max(0.0, rotatedPos.z);  // How much this vertex is in the tail (0 at head)
        float stretchMultiplier = uTrailStretch * 4.0;  // 4x longer trail (was 2x)
        float stretchAmount = tailAmount * stretchMultiplier;
        vec3 stretchOffset = -instanceFallDir * stretchAmount;  // Stretch backward along travel

        vec3 scaledPos = rotatedPos * uScale;
        worldPos = worldCometPos + scaledPos + stretchOffset * uScale;

        // Pass comet center and trail length for fragment shader distance-based fade
        vCometCenter = worldCometPos;
        vTrailLength = stretchMultiplier * uScale * 1.5;  // Approximate max trail length

        // Compute fade based on how far this vertex was stretched
        // stretchAmount represents how far back the vertex moved from center
        // This gives proper gradient along the entire stretched trail
        float maxStretchAmount = stretchMultiplier * 0.5;  // Max possible stretch
        vTrailFade = clamp(stretchAmount / maxStretchAmount, 0.0, 1.0);
    }

    vFragPos = worldPos;
    vNormal = aNormal;
    vTexCoord = aTexCoord;
    vLocalPos = aPos;  // Pass local position for axis coloring

    gl_Position = uProjection * uView * vec4(worldPos, 1.0);

    // Clamp depth to just inside far plane (avoids clipping) while still allowing occlusion
    // gl_Position.z/w gives NDC depth. We clamp it to 0.9999 of far plane max
    // This keeps comets visible but still behind any closer geometry
    gl_Position.z = min(gl_Position.z, gl_Position.w * 0.9999);
}
