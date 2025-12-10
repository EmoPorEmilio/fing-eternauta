#version 450 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uCurrentFrame;   // Current rendered frame
uniform sampler2D uPreviousFrame;  // Previous accumulated frame
uniform float uBlendFactor;        // How much to blend (0.0 = only current, 1.0 = only previous)

void main()
{
    vec3 current = texture(uCurrentFrame, vTexCoord).rgb;
    vec3 previous = texture(uPreviousFrame, vTexCoord).rgb;

    // Blend current frame with accumulated previous frame
    // Higher blend factor = more motion blur trail
    vec3 blended = mix(current, previous, uBlendFactor);

    FragColor = vec4(blended, 1.0);
}
