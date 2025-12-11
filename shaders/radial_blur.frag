#version 450 core

// Radial blur effect - creates dramatic "tunnel vision" or "zoom" blur
// Works independently of camera movement

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uColorBuffer;
uniform float uBlurStrength;    // 0.0 = no blur, 1.0 = max blur
uniform vec2 uCenter;           // Center of blur (usually 0.5, 0.5)
uniform int uNumSamples;        // Number of samples (e.g., 16)

void main()
{
    vec2 dir = vTexCoord - uCenter;
    float dist = length(dir);

    // Blur increases with distance from center (tunnel vision effect)
    float blurAmount = dist * uBlurStrength * 0.1;

    vec3 color = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < uNumSamples; ++i) {
        float t = float(i) / float(uNumSamples - 1) - 0.5;  // -0.5 to 0.5
        vec2 offset = dir * t * blurAmount;
        vec2 sampleUV = clamp(vTexCoord - offset, vec2(0.001), vec2(0.999));

        // Weight samples by distance from center (center is sharper)
        float weight = 1.0 - abs(t);
        color += texture(uColorBuffer, sampleUV).rgb * weight;
        totalWeight += weight;
    }

    color /= totalWeight;

    // Vignette: dim edges with black overlay (80% opacity at edges, 0% at center)
    // dist ranges from 0 (center) to ~0.7 (corners)
    float vignetteStrength = smoothstep(0.2, 0.8, dist) * 0.8;  // 0 at center, up to 0.8 at edges
    color = mix(color, vec3(0.0), vignetteStrength);

    FragColor = vec4(color, 1.0);
}
