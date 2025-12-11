#version 330 core

in vec2 vLocalPos;

out vec4 FragColor;

void main() {
    // Calculate distance from center (0,0)
    float dist = length(vLocalPos);

    // Discard pixels outside the circle
    if (dist > 1.0) {
        discard;
    }

    // Soft edge falloff - more transparent at edges
    float alpha = 0.1 * (1.0 - smoothstep(0.7, 1.0, dist));

    FragColor = vec4(1.0, 0.0, 0.0, alpha);  // Red with 10% max opacity
}
