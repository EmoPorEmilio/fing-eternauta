#version 450 core
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vFragPos;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform int uHasTexture;

// Fog parameters
const vec3 fogColor = vec3(0.1, 0.1, 0.12);  // Match background
const float fogDensity = 0.08;
const float fogDesaturation = 0.8;  // How much to desaturate at max fog

out vec4 FragColor;

// Convert to grayscale for desaturation
float luminance(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    float ambient = 0.3;
    float diff = max(dot(normal, lightDir), 0.0);
    float light = ambient + diff * 0.7;

    vec3 color;
    if (uHasTexture == 1)
    {
        color = texture(uTexture, vTexCoord).rgb;
    }
    else
    {
        color = vec3(1.0, 1.0, 1.0);
    }

    vec3 litColor = color * light;

    // Exponential squared fog
    float distance = length(vFragPos - uViewPos);
    float fogFactor = 1.0 - exp(-pow(fogDensity * distance, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    // Desaturate based on fog amount
    float gray = luminance(litColor);
    vec3 desaturated = mix(litColor, vec3(gray), fogFactor * fogDesaturation);

    // Blend with fog color
    vec3 finalColor = mix(desaturated, fogColor, fogFactor);

    FragColor = vec4(finalColor, 1.0);
}
