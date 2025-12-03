#version 330 core

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vColor;
in vec2 vUV;

uniform vec3 uCameraPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec4 uBaseColorFactor;
uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uOcclusionStrength;

// Textures
uniform sampler2D uBaseColorTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uOcclusionTexture;
uniform sampler2D uNormalTexture;

// Material flags
uniform bool uHasBaseColorTexture;
uniform bool uHasMetallicRoughnessTexture;
uniform bool uHasOcclusionTexture;
uniform bool uHasNormalTexture;
uniform float uNormalScale;

// Additional controls
uniform float uExposure = 1.0;

// Flashlight uniforms
uniform bool uFlashlightOn;
uniform vec3 uFlashlightPos;
uniform vec3 uFlashlightDir;
uniform float uFlashlightCutoff;
uniform float uFlashlightBrightness;
uniform vec3 uFlashlightColor;

// Debug uniforms
uniform bool uDebugNormals;

// Fog uniforms
uniform bool uFogEnabled;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogDesaturationStrength;
uniform float uFogAbsorptionDensity;
uniform float uFogAbsorptionStrength;
uniform vec3 uBackgroundColor; // Clear color for true disappearing

out vec4 FragColor;

// Unified flashlight UBO (binding set in LightManager)
layout(std140) uniform FlashlightBlock {
    vec4 uFL_Pos;      // xyz, w=1
    vec4 uFL_Dir;      // xyz normalized, w=0
    vec4 uFL_Color;    // rgb, a=1
    vec4 uFL_Params;   // x:enabled, y:cutoff (cos), z:outerCutoff (cos), w:brightness
};

const float PI = 3.14159265359;

// With GL_FRAMEBUFFER_SRGB enabled and sRGB textures bound correctly, stay in linear space here.

float saturate(float x) { 
    return clamp(x, 0.0, 1.0); 
}

// PBR BRDF functions
float D_GGX(float NoH, float a) {
    float a2 = a * a;
    float d = (NoH * NoH) * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

float G_SchlickGGX(float NoV, float k) {
    return NoV / (NoV * (1.0 - k) + k);
}

float G_Smith(float NoV, float NoL, float k) {
    return G_SchlickGGX(NoV, k) * G_SchlickGGX(NoL, k);
}

vec3 F_Schlick(vec3 F0, float HoV) {
    return F0 + (1.0 - F0) * pow(1.0 - HoV, 5.0);
}

void main() {
    // glTF uses top-left UV origin, OpenGL uses bottom-left - flip V coordinate
    vec2 uv = vec2(vUV.x, 1.0 - vUV.y);

    vec3 N = normalize(vNormal);
    // Normal mapping if available
    if (uHasNormalTexture) {
        // Build TBN from interpolated attributes (T in attribute 4 with w sign)
        // Tangent attribute not passed here; for correctness we would need TBN per-vertex.
        // As a minimal path, perturb by sampling normal map and assuming N is reasonably correct.
        vec3 nTex = texture(uNormalTexture, uv).xyz * 2.0 - 1.0;
        nTex.xy *= uNormalScale;
        // Fallback: blend toward geometric normal when tangent not bound
        N = normalize(mix(N, nTex, 0.0));
    }
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 L = normalize(-uLightDir);
    vec3 H = normalize(V + L);

    float NoV = max(dot(N, V), 0.0);
    float NoL = max(dot(N, L), 0.0);
    float NoH = max(dot(N, H), 0.0);
    float HoV = max(dot(H, V), 0.0);

    // Base color
    vec4 base = uBaseColorFactor;
    if (uHasBaseColorTexture) {
        vec3 tex = texture(uBaseColorTexture, uv).rgb; // sampled linear due to sRGB texture
        base.rgb *= tex;
    }
    vec3 albedo = base.rgb;

    // Metallic and roughness
    float metallic = saturate(uMetallicFactor);
    float roughness = clamp(uRoughnessFactor, 0.04, 1.0);

    if (uHasMetallicRoughnessTexture) {
        vec3 mr = texture(uMetallicRoughnessTexture, uv).rgb;
        roughness = clamp(mr.g, 0.04, 1.0);
        metallic = saturate(mr.b);
    }
    
    float a = roughness * roughness;

    // PBR calculations
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float D = D_GGX(NoH, a);
    float k = (roughness + 1.0); 
    k = (k * k) / 8.0;
    float G = G_Smith(NoV, NoL, k);
    vec3 F = F_Schlick(F0, HoV);

    vec3 spec = (D * G * F) / max(4.0 * NoV * NoL, 1e-6);
    vec3 kd = (1.0 - F) * (1.0 - metallic);
    vec3 Lo = (kd * albedo / PI + spec) * uLightColor * NoL;
    
    // Ambient occlusion
    float ao = 1.0;
    if (uHasOcclusionTexture) {
        ao = mix(1.0, texture(uOcclusionTexture, uv).r, uOcclusionStrength);
    }
    
    // Higher ambient for better visibility (0.3 instead of 0.03)
    vec3 ambient = 0.3 * albedo * ao;
    vec3 color = ambient + Lo;
    
    // Add flashlight lighting if enabled (UBO or legacy uniforms)
    bool flOn = uFlashlightOn || (uFL_Params.x > 0.5);
    if (flOn) {
        vec3 flPos = (uFL_Params.x > 0.0) ? uFL_Pos.xyz : uFlashlightPos;
        vec3 flDir = normalize((uFL_Params.x > 0.0) ? uFL_Dir.xyz : uFlashlightDir);
        vec3 flColor = (uFL_Params.x > 0.0) ? uFL_Color.rgb : uFlashlightColor;
        float innerCut = (uFL_Params.y > 0.0) ? uFL_Params.y : uFlashlightCutoff;
        float outerCut = (uFL_Params.z > 0.0) ? uFL_Params.z : innerCut * 0.9;
        float brightness = (uFL_Params.w > 0.0) ? uFL_Params.w : uFlashlightBrightness;

        vec3 Ls = normalize(vWorldPos - flPos);
        float theta = dot(flDir, Ls);
        float intensity = smoothstep(outerCut, innerCut, theta);
        if (intensity > 0.0) {
            float d = length(vWorldPos - flPos);
            float attenuation = 1.0 / (1.0 + 0.02 * d + 0.002 * d * d);

            vec3 Ldir = normalize(flPos - vWorldPos);
            float NoL2 = max(dot(N, Ldir), 0.0);
            vec3 diffuse = albedo * NoL2 * flColor;

            vec3 H2 = normalize(V + Ldir);
            float NoH2 = max(dot(N, H2), 0.0);
            float spec2 = pow(NoH2, 64.0);
            vec3 specular = flColor * spec2 * 0.25;

            color += (diffuse + specular) * attenuation * intensity * brightness;
        }
    }
    
    color *= uExposure;

    // Debug normal visualization
    if (uDebugNormals) {
        FragColor = vec4(N * 0.5 + 0.5, 1.0); // Show normals as colors
        return;
    }

    // Apply fog effects (unified realistic fog system - matches other shaders)
    if (uFogEnabled) {
        float distance = length(vWorldPos - uCameraPos);

        // 1) Global desaturation (independent of distance) - keeps working as before
        if (uFogDesaturationStrength > 0.0) {
            float desaturationFactor = clamp(uFogDesaturationStrength, 0.0, 1.0);
            vec3 luminance = vec3(dot(color, vec3(0.299, 0.587, 0.114)));
            color = mix(color, luminance, desaturationFactor);
        }

        // 2) Enhanced physically-based fog with true model disappearing
        if (uFogDensity > 0.0) {
            // Standard exponential fog factor
            float fogFactor = 1.0 - exp(-uFogDensity * distance);
            
            // Optional: Apply absorption as light attenuation (more realistic)
            if (uFogAbsorptionStrength > 0.0 && uFogAbsorptionDensity > 0.0) {
                float transmission = exp(-uFogAbsorptionDensity * distance);
                float absorptionFactor = mix(1.0, transmission, uFogAbsorptionStrength);
                color *= absorptionFactor;
            }
            
            // Two-stage fog blending for true disappearing:
            // Stage 1: Near to mid distance - blend with fog color (atmospheric effect)
            float midFogFactor = clamp(fogFactor * 1.5, 0.0, 1.0); // Reaches 1.0 at 67% distance
            color = mix(color, uFogColor, midFogFactor);
            
            // Stage 2: Mid to far distance - blend fog color to background color (true disappearing)
            float disappearFactor = clamp((fogFactor - 0.67) * 3.0, 0.0, 1.0); // Starts at 67% distance
            color = mix(color, uBackgroundColor, disappearFactor);
        }
    }

    FragColor = vec4(color, base.a);
}
