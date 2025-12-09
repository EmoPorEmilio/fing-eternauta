#version 450 core
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vFragPos;
in vec3 vColor;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uViewPos;

// Fog parameters (same as model.frag)
const vec3 fogColor = vec3(0.1, 0.1, 0.12);
const float fogDensity = 0.08;
const float fogDesaturation = 0.8;

out vec4 FragColor;

float luminance(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
    // vColor is RED (1,0,0) for deformed, WHITE (1,1,1) for normal
    // Use green channel to detect: g=0 means deformed, g=1 means normal
    float deformed = 1.0 - vColor.g;  // 1.0 if deformed, 0.0 if normal

    // Bright red for normal, dark red for deformed
    vec3 color = mix(vec3(1.0, 0.0, 0.0), vec3(0.3, 0.0, 0.0), deformed);

    FragColor = vec4(color, 1.0);
}
