#version 450 core
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vFragPos;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform int uHasTexture;

out vec4 FragColor;

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
        color = vec3(0.8, 0.8, 0.8);
    }

    FragColor = vec4(color * light, 1.0);
}
