#version 450 core
in vec3 FragPos;
in vec3 Normal;
in vec2 vUV;

out vec4 FragColor;

uniform vec3 uObjectColor;
uniform sampler2D uTex;
uniform bool uUseTexture;
uniform vec3 uViewPos;
// no fragment distance culling in instanced path
uniform bool uFlashlightOn;
uniform vec3 uFlashlightPos;
uniform vec3 uFlashlightDir;
uniform float uFlashlightCutoff;
uniform float uFlashlightBrightness;
uniform vec3 uFlashlightColor;

// Fog uniforms
uniform bool uFogEnabled;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogDesaturationStrength;
uniform float uFogAbsorptionDensity;
uniform float uFogAbsorptionStrength;
uniform vec3 uBackgroundColor; // Clear color for true disappearing

void main()
{
    // Very low ambient lighting to make flashlight effect more pronounced
    vec3 ambient = vec3(0.01);
    
    // Basic lighting with distance attenuation
    vec3 norm = normalize(Normal);
    vec3 lightPos = vec3(0.0, 10.0, 0.0);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    
    // Add distance attenuation for more realistic lighting
    float lightDistance = length(lightPos - FragPos);
    float attenuation = 1.0 / (1.0 + 0.1 * lightDistance + 0.01 * lightDistance * lightDistance);
    
    vec3 diffuse = diff * vec3(0.8) * attenuation;
    
    // Flashlight lighting
    vec3 flashlightContribution = vec3(0.0);
    if (uFlashlightOn) {
        vec3 flashlightDir = normalize(uFlashlightDir);
        vec3 lightToFrag = normalize(FragPos - uFlashlightPos);
        float theta = dot(lightToFrag, flashlightDir);
        
        if (theta > uFlashlightCutoff) {
            float intensity = (theta - uFlashlightCutoff) / (1.0 - uFlashlightCutoff);
            intensity = clamp(intensity, 0.0, 1.0);
            
            float flashlightDistance = length(FragPos - uFlashlightPos);
            float flashlightAttenuation = 1.0 / (1.0 + 0.02 * flashlightDistance + 0.002 * flashlightDistance * flashlightDistance);
            
            vec3 flashlightDiffuse = max(dot(norm, -lightToFrag), 0.0) * uFlashlightColor * intensity * flashlightAttenuation * uFlashlightBrightness;
            flashlightContribution = flashlightDiffuse;
        }
    }
    
    vec3 result = (ambient + diffuse + flashlightContribution) * uObjectColor;
    
    // Apply fog effects (unified realistic fog system)
    if (uFogEnabled) {
        float distance = length(FragPos - uViewPos);

        // 1) Global desaturation (independent of distance) - keeps working as before
        if (uFogDesaturationStrength > 0.0) {
            float desaturationFactor = clamp(uFogDesaturationStrength, 0.0, 1.0);
            vec3 luminance = vec3(dot(result, vec3(0.299, 0.587, 0.114)));
            result = mix(result, luminance, desaturationFactor);
        }

        // 2) Enhanced physically-based fog with true object disappearing
        if (uFogDensity > 0.0) {
            // Standard exponential fog factor
            float fogFactor = 1.0 - exp(-uFogDensity * distance);
            
            // Optional: Apply absorption as light attenuation (more realistic)
            if (uFogAbsorptionStrength > 0.0 && uFogAbsorptionDensity > 0.0) {
                float transmission = exp(-uFogAbsorptionDensity * distance);
                float absorptionFactor = mix(1.0, transmission, uFogAbsorptionStrength);
                result *= absorptionFactor;
            }
            
            // Two-stage fog blending for true disappearing:
            // Stage 1: Near to mid distance - blend with fog color (atmospheric effect)
            float midFogFactor = clamp(fogFactor * 1.5, 0.0, 1.0); // Reaches 1.0 at 67% distance
            result = mix(result, uFogColor, midFogFactor);
            
            // Stage 2: Mid to far distance - blend fog color to background color (true disappearing)
            float disappearFactor = clamp((fogFactor - 0.67) * 3.0, 0.0, 1.0); // Starts at 67% distance
            result = mix(result, uBackgroundColor, disappearFactor);
        }
    }
    
    FragColor = vec4(result, 1.0);
}
