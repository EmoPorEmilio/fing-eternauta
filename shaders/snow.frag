#version 450 core

in vec2 vUV;
flat in float vSeed;

out vec4 FragColor;

// Simple hash for pseudo-random values
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// 2D noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

void main() {
    // Center UV coordinates: -0.5 to 0.5
    vec2 center = vUV - 0.5;
    float dist = length(center);

    // Get angle for radial noise
    float angle = atan(center.y, center.x);

    // STEP 1: Create noisy blob boundary
    // Multiple frequencies of sine waves create organic edge
    float blobNoise = sin(angle * 3.0 + vSeed * 50.0) * 0.12;
    blobNoise += sin(angle * 5.0 + vSeed * 137.0) * 0.08;
    blobNoise += sin(angle * 7.0 + vSeed * 251.0) * 0.05;

    // Blob radius varies with noise (base radius ~0.30)
    float blobRadius = 0.30 + blobNoise;

    // Discard pixels outside the noisy blob shape
    if (dist > blobRadius) discard;

    // STEP 2: Opacity gradient from edge to center
    // Normalize distance relative to blob boundary (0 at center, 1 at edge)
    float normalizedDist = dist / blobRadius;

    // Smooth falloff from center (opaque) to edge (transparent)
    float alpha = 1.0 - smoothstep(0.0, 1.0, normalizedDist);

    // Soften further with power curve
    alpha = alpha * alpha;

    // White color with slight blue tint
    vec3 color = vec3(0.95, 0.97, 1.0);

    FragColor = vec4(color, alpha * 0.6);
}
