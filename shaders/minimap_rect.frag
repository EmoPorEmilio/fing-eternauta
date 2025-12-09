#version 450 core
in vec2 vScreenPos;

uniform vec2 uMinimapCenter;
uniform float uMinimapRadius;
uniform vec4 uColor;

out vec4 FragColor;

void main()
{
    // Calculate distance from minimap center
    float dist = length(vScreenPos - uMinimapCenter);

    // Discard fragments outside the minimap circle
    if (dist > uMinimapRadius) {
        discard;
    }

    FragColor = uColor;
}
