#version 450 core

// GPU Gems 3 Chapter 27: Motion Blur as a Post-Processing Effect
// https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uColorBuffer;      // Current rendered frame
uniform sampler2D uDepthBuffer;      // Depth buffer for world position reconstruction
uniform mat4 uViewProjection;        // Current frame's view-projection matrix
uniform mat4 uPrevViewProjection;    // Previous frame's view-projection matrix
uniform mat4 uInvViewProjection;     // Inverse of current view-projection
uniform float uBlurStrength;         // Scale factor for blur amount (0 = no blur)
uniform int uNumSamples;             // Number of samples along velocity vector (e.g., 8)

void main()
{
    // Step 1: Get depth at this pixel
    float depth = texture(uDepthBuffer, vTexCoord).r;

    // Step 2: Reconstruct clip-space position
    // Convert UV (0-1) to NDC (-1 to 1), with depth
    vec4 clipPos = vec4(vTexCoord * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);

    // Step 3: Transform to world space using inverse view-projection
    vec4 worldPos = uInvViewProjection * clipPos;
    worldPos /= worldPos.w;  // Perspective divide

    // Step 4: Project world position using PREVIOUS frame's view-projection
    vec4 prevClipPos = uPrevViewProjection * worldPos;
    prevClipPos /= prevClipPos.w;  // Perspective divide

    // Step 5: Compute velocity in screen space (NDC difference)
    // This is where the pixel WAS in the previous frame vs where it IS now
    vec2 velocity = (clipPos.xy - prevClipPos.xy) * 0.5;  // NDC to UV scale

    // Scale velocity by blur strength
    velocity *= uBlurStrength;

    // Clamp velocity to avoid extreme blur (max ~15% of screen per frame)
    float maxVelocity = 0.15;
    float velocityLen = length(velocity);
    if (velocityLen > maxVelocity) {
        velocity = velocity * (maxVelocity / velocityLen);
    }

    // Step 6: Sample along the velocity vector and accumulate
    vec3 color = vec3(0.0);

    for (int i = 0; i < uNumSamples; ++i) {
        // Sample from current position toward where the pixel came from
        float t = float(i) / float(uNumSamples - 1) - 0.5;  // -0.5 to 0.5 (centered)
        vec2 sampleUV = vTexCoord + velocity * t;

        // Clamp to valid UV range
        sampleUV = clamp(sampleUV, vec2(0.001), vec2(0.999));

        color += texture(uColorBuffer, sampleUV).rgb;
    }

    color /= float(uNumSamples);

    FragColor = vec4(color, 1.0);
}
