#version 450 core

in vec2 vUV;
out vec4 FragColor;

uniform vec4 uColor;

void main() {
    float dist = length(vUV);
    if (dist > 1.0) {
        discard;
    }

    // Smooth edge for anti-aliasing
    float alpha = 1.0 - smoothstep(0.8, 1.0, dist);
    FragColor = vec4(uColor.rgb, uColor.a * alpha);
}
