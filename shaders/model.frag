#version 450 core
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vFragPos;
in vec4 vFragPosLightSpace;
in vec3 vWorldNormal;

uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform sampler2D uShadowMap;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform int uHasTexture;
uniform int uHasNormalMap;
uniform int uFogEnabled;
uniform int uShadowsEnabled;
uniform int uTriplanarMapping;  // Use world-space UV projection
uniform float uTextureScale;    // How many world units per texture repeat (default 4.0)

// Fog parameters (configurable via uniforms, with defaults matching GameConfig)
uniform vec3 uFogColor;         // Default: vec3(0.5, 0.5, 0.55)
uniform float uFogDensity;      // Default: 0.02
uniform float uFogDesaturation; // Default: 0.8

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

    // Bias to prevent shadow acne (kept small for accurate small-scale character shadows)
    float bias = max(0.0005 * (1.0 - dot(normal, lightDir)), 0.0001);

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

// Perturb normal using normal map sample (tangent space to world space for triplanar)
vec3 perturbNormalTriplanar(vec3 normalSample, vec3 surfaceNormal, vec3 blendWeights)
{
    // Convert from [0,1] to [-1,1]
    vec3 tangentNormal = normalSample * 2.0 - 1.0;

    // For triplanar, we blend the normal perturbations based on the dominant axis
    // This is a simplified approach - proper triplanar normals would need full TBN per axis
    vec3 absNormal = abs(surfaceNormal);

    vec3 perturbedNormal = surfaceNormal;

    // Apply perturbation based on which axis dominates
    if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
        // X-dominant face (left/right walls)
        perturbedNormal = normalize(vec3(
            surfaceNormal.x,
            surfaceNormal.y + tangentNormal.y * 0.5,
            surfaceNormal.z + tangentNormal.x * 0.5 * sign(surfaceNormal.x)
        ));
    } else if (absNormal.y > absNormal.z) {
        // Y-dominant face (top/bottom)
        perturbedNormal = normalize(vec3(
            surfaceNormal.x + tangentNormal.x * 0.5,
            surfaceNormal.y,
            surfaceNormal.z + tangentNormal.y * 0.5
        ));
    } else {
        // Z-dominant face (front/back walls)
        perturbedNormal = normalize(vec3(
            surfaceNormal.x + tangentNormal.x * 0.5 * sign(surfaceNormal.z),
            surfaceNormal.y + tangentNormal.y * 0.5,
            surfaceNormal.z
        ));
    }

    return perturbedNormal;
}

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);

    vec3 color = vec3(1.0);
    float scale = uTextureScale > 0.0 ? uTextureScale : 4.0;

    // Sample textures and normal map with triplanar or regular UV
    if (uHasTexture == 1)
    {
        if (uTriplanarMapping == 1)
        {
            // Calculate blend weights based on normal direction
            vec3 blendWeights = abs(vWorldNormal);
            blendWeights = blendWeights / (blendWeights.x + blendWeights.y + blendWeights.z);

            // Sample diffuse texture from each axis projection
            vec3 xProjection = texture(uTexture, vFragPos.zy / scale).rgb;
            vec3 yProjection = texture(uTexture, vFragPos.xz / scale).rgb;
            vec3 zProjection = texture(uTexture, vFragPos.xy / scale).rgb;

            color = xProjection * blendWeights.x +
                    yProjection * blendWeights.y +
                    zProjection * blendWeights.z;

            // Desaturate building textures by 50%
            float gray = luminance(color);
            color = mix(color, vec3(gray), 0.5);

            // Sample and apply normal map if available
            if (uHasNormalMap == 1)
            {
                vec3 xNormal = texture(uNormalMap, vFragPos.zy / scale).rgb;
                vec3 yNormal = texture(uNormalMap, vFragPos.xz / scale).rgb;
                vec3 zNormal = texture(uNormalMap, vFragPos.xy / scale).rgb;

                vec3 blendedNormalSample = xNormal * blendWeights.x +
                                           yNormal * blendWeights.y +
                                           zNormal * blendWeights.z;

                normal = perturbNormalTriplanar(blendedNormalSample, normal, blendWeights);
            }
        }
        else
        {
            color = texture(uTexture, vTexCoord).rgb;

            if (uHasNormalMap == 1)
            {
                vec3 normalSample = texture(uNormalMap, vTexCoord).rgb;
                vec3 tangentNormal = normalSample * 2.0 - 1.0;
                // Simple normal perturbation for non-triplanar (assumes roughly axis-aligned surfaces)
                normal = normalize(normal + vec3(tangentNormal.x, tangentNormal.y, 0.0) * 0.5);
            }
        }
    }

    float ambient = 0.3;
    float diff = max(dot(normal, lightDir), 0.0);

    // Shadow calculation
    float shadow = 0.0;
    if (uShadowsEnabled == 1) {
        shadow = calculateShadow(vFragPosLightSpace, normal, lightDir);
    }

    // Shadow reduces diffuse lighting, ambient stays constant
    float light = ambient + diff * 0.7 * (1.0 - shadow);

    vec3 litColor = color * light;

    vec3 finalColor = litColor;

    if (uFogEnabled == 1) {
        // Use uniform density, fallback to default if not set
        float density = uFogDensity > 0.0 ? uFogDensity : 0.02;
        float desaturation = uFogDesaturation > 0.0 ? uFogDesaturation : 0.8;
        vec3 fogCol = (uFogColor.r + uFogColor.g + uFogColor.b) > 0.0 ? uFogColor : vec3(0.15, 0.15, 0.17);

        // Exponential squared fog
        float distance = length(vFragPos - uViewPos);
        float fogFactor = 1.0 - exp(-pow(density * distance, 2.0));
        fogFactor = clamp(fogFactor, 0.0, 1.0);

        // Desaturate based on fog amount
        float gray = luminance(litColor);
        vec3 desaturated = mix(litColor, vec3(gray), fogFactor * desaturation);

        // Blend with fog color
        finalColor = mix(desaturated, fogCol, fogFactor);
    }

    FragColor = vec4(finalColor, 1.0);
}
