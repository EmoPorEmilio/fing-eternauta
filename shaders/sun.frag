#version 450 core
in vec2 vUV;
out vec4 FragColor;

void main() {
    vec2 center = vec2(0.5);
    float dist = length(vUV - center) * 2.0;  // 0 at center, 1 at edge

    // Soft circular gradient with glow effect
    float core = 1.0 - smoothstep(0.0, 0.3, dist);   // Bright white core
    float glow = 1.0 - smoothstep(0.2, 1.0, dist);   // Soft outer glow

    // Warm white-yellow sun color
    vec3 sunColor = vec3(1.0, 0.95, 0.8);

    // Combine core brightness with softer glow
    float alpha = core * 1.0 + glow * 0.5;

    // Discard fully transparent pixels
    if (alpha < 0.01) discard;

    FragColor = vec4(sunColor, alpha);
}
