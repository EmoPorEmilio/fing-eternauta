#version 450 core
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vFragPos;
in vec4 vFragPosLightSpace;
in vec3 vWorldNormal;

uniform sampler2D uTexture;
uniform sampler2D uShadowMap;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform int uHasTexture;
uniform int uFogEnabled;
uniform int uShadowsEnabled;
uniform int uTriplanarMapping;  // Use world-space UV projection
uniform float uTextureScale;    // How many world units per texture repeat (default 4.0)

// Fog parameters
const vec3 fogColor = vec3(0.5, 0.5, 0.55);  // Gray - match snowy atmosphere
const float fogDensity = 0.02;
const float fogDesaturation = 0.8;  // How much to desaturate at max fog

out vec4 FragColor;

// Convert to grayscale for desaturation
float luminance(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

// Poisson disk samples for smooth shadow sampling
const vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
);

// Calculate soft shadow using Poisson disk sampling
float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;  // Transform to [0,1] range

    // Outside shadow map bounds - no shadow
    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float currentDepth = projCoords.z;

    // Bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);

    // Poisson disk sampling for smooth soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    float spreadRadius = 4.0;  // How spread out the samples are

    for (int i = 0; i < 16; ++i) {
        vec2 offset = poissonDisk[i] * texelSize * spreadRadius;
        float pcfDepth = texture(uShadowMap, projCoords.xy + offset).r;
        shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
    shadow /= 16.0;

    return shadow;
}

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    float ambient = 0.3;
    float diff = max(dot(normal, lightDir), 0.0);

    // Shadow calculation
    float shadow = 0.0;
    if (uShadowsEnabled == 1) {
        shadow = calculateShadow(vFragPosLightSpace, normal, lightDir);
    }

    // Shadow reduces diffuse lighting, ambient stays constant
    float light = ambient + diff * 0.7 * (1.0 - shadow);

    vec3 color;
    if (uHasTexture == 1)
    {
        if (uTriplanarMapping == 1)
        {
            // Triplanar mapping: project texture based on world position and normal
            float scale = uTextureScale > 0.0 ? uTextureScale : 4.0;  // Default 4 units per repeat

            // Calculate blend weights based on normal direction
            vec3 blendWeights = abs(vWorldNormal);
            blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

            // Sample texture from each axis projection
            vec3 xProjection = texture(uTexture, vFragPos.zy / scale).rgb;  // Project along X
            vec3 yProjection = texture(uTexture, vFragPos.xz / scale).rgb;  // Project along Y (top/bottom)
            vec3 zProjection = texture(uTexture, vFragPos.xy / scale).rgb;  // Project along Z

            // Blend based on normal direction
            color = xProjection * blendWeights.x +
                    yProjection * blendWeights.y +
                    zProjection * blendWeights.z;
        }
        else
        {
            color = texture(uTexture, vTexCoord).rgb;
        }
    }
    else
    {
        color = vec3(1.0, 1.0, 1.0);
    }

    vec3 litColor = color * light;

    vec3 finalColor = litColor;

    if (uFogEnabled == 1) {
        // Exponential squared fog
        float distance = length(vFragPos - uViewPos);
        float fogFactor = 1.0 - exp(-pow(fogDensity * distance, 2.0));
        fogFactor = clamp(fogFactor, 0.0, 1.0);

        // Desaturate based on fog amount
        float gray = luminance(litColor);
        vec3 desaturated = mix(litColor, vec3(gray), fogFactor * fogDesaturation);

        // Blend with fog color
        finalColor = mix(desaturated, fogColor, fogFactor);
    }

    FragColor = vec4(finalColor, 1.0);
}
