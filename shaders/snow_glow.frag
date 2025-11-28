#version 400 core

in vec3 vColor;
in vec3 vWorldPos;
in vec2 vLocal;
in vec2 vVelocity;

uniform int useBillboard;         // 1 when quads face camera in VS
uniform int useDisc;              // 1 for particle quads, 0 for static geometry
uniform float baseAlpha;          // 0..1 transparency
uniform float trailOpacity;       // motion blur trail opacity (1.0 for normal flakes, <1.0 for trails)
uniform float instanceAge;        // 0..1 when used for puffs (age normalized)

// Fog uniforms
uniform bool uFogEnabled;
uniform vec3 uFogColor;
uniform float uFogDensity;
uniform float uFogDesaturationStrength;
uniform float uFogAbsorptionDensity;
uniform float uFogAbsorptionStrength;
uniform vec3 uCameraPos;
uniform vec3 uBackgroundColor; // Clear color for true disappearing

out vec4 FragColor;

void main() {
    vec3 color;
    float alpha;
    
    // Simple white billboard mode (no disc, no lighting) – minimal viable snow
    if (useBillboard == 1 && useDisc == 0)
    {
        color = vec3(1.0) * max(trailOpacity, 0.0);
        alpha = max(trailOpacity, 0.0) * baseAlpha * (1.0 - instanceAge);
    }
    // For future use: disc mode with lighting
    else if (useDisc == 1)
    {
        // Circular mask and fake sphere normal from quad-local coords
        vec2 uv = vLocal; // [-1,1]
        float r2 = dot(uv, uv);
        if (r2 > 1.0) discard;
        
        // Simple white disc
        color = vec3(1.0) * max(trailOpacity, 0.0);
        alpha = max(trailOpacity, 0.0) * baseAlpha;
    }
    // Fallback for static geometry
    else
    {
        color = vColor * max(trailOpacity, 0.0);
        alpha = max(trailOpacity, 0.0) * baseAlpha;
    }
    
    // Apply fog effects (unified realistic fog system - matches object_instanced.frag)
    if (uFogEnabled) {
        float distance = length(vWorldPos - uCameraPos);

        // 1) Global desaturation (independent of distance) - keeps working as before
        if (uFogDesaturationStrength > 0.0) {
            float desaturationFactor = clamp(uFogDesaturationStrength, 0.0, 1.0);
            vec3 luminance = vec3(dot(color, vec3(0.299, 0.587, 0.114)));
            color = mix(color, luminance, desaturationFactor);
        }

        // 2) Enhanced physically-based fog with true particle disappearing
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
    
    FragColor = vec4(color, alpha);
}
