#version 450 core
in vec3 FragPos;
in vec3 Normal;
in vec2 vUV;

out vec4 FragColor;

uniform vec3 uLightPos;
uniform vec3 uViewPos;
uniform vec3 uLightColor;
uniform vec3 uObjectColor;
uniform sampler2D uAlbedoTex;
uniform sampler2D uRoughnessTex;
uniform sampler2D uTranslucencyTex;
uniform sampler2D uHeightTex; // used as height for normal mapping
uniform float uNormalStrength; // e.g., 1.0
uniform vec2 uWorldPerUV; // world units per 1.0 UV in (u,v)
uniform mat4 uModel;
uniform float uAmbient;            // 0..0.5 typical
uniform float uSpecularStrength;   // 0..1
uniform float uRoughnessBias;      // -0.3..0.3
uniform float uCullDistance;
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

// Unified flashlight UBO (binding set in LightManager)
layout(std140) uniform FlashlightBlock {
    vec4 uFL_Pos;      // xyz, w=1
    vec4 uFL_Dir;      // xyz normalized, w=0
    vec4 uFL_Color;    // rgb, a=1
    vec4 uFL_Params;   // x:enabled, y:cutoff (cos), z:outerCutoff (cos), w:brightness
};

void main()
{
    vec3 ambient = uAmbient * uLightColor;

    // Derive a tangent frame for the floor (aligned with world XZ)
    vec3 Ngeom = normalize(Normal);
    vec3 T = normalize((uModel * vec4(1.0, 0.0, 0.0, 0.0)).xyz);
    // Orthonormalize T against N, then compute B
    T = normalize(T - Ngeom * dot(T, Ngeom));
    vec3 B = normalize(cross(Ngeom, T));
    // Compute height gradients in UV, convert to world-space bump
    ivec2 ts = textureSize(uHeightTex, 0);
    vec2 texel = 1.0 / vec2(ts);
    float h = texture(uHeightTex, vUV).r;
    float hx = texture(uHeightTex, vUV + vec2(texel.x, 0.0)).r;
    float hy = texture(uHeightTex, vUV + vec2(0.0, texel.y)).r;
    float H = (h - 0.5) * 2.0;
    float Hx = (hx - 0.5) * 2.0;
    float Hy = (hy - 0.5) * 2.0;
    float dHdu = (Hx - H) / max(texel.x, 1e-4);
    float dHdv = (Hy - H) / max(texel.y, 1e-4);
    vec3 bump = (dHdu / max(uWorldPerUV.x, 1e-4)) * T + (dHdv / max(uWorldPerUV.y, 1e-4)) * B;
    vec3 norm = normalize(Ngeom - uNormalStrength * bump);
	vec3 lightDir = normalize(uLightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * uLightColor;

	float specularStrength = 0.4;
    vec3 viewDir = normalize(uViewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
    float roughness = clamp(texture(uRoughnessTex, vUV).r + uRoughnessBias, 0.0, 1.0);
    float shininess = mix(16.0, 128.0, pow(1.0 - roughness, 1.5));
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = uSpecularStrength * spec * uLightColor;

	vec3 flashlightContribution = vec3(0.0);
	if (uFlashlightOn) {
		vec3 flashlightDir = normalize(uFlashlightDir);
		vec3 lightToFrag = normalize(FragPos - uFlashlightPos);
		float theta = dot(lightToFrag, flashlightDir);
		if (theta > uFlashlightCutoff) {
			float intensity = (theta - uFlashlightCutoff) / (1.0 - uFlashlightCutoff);
			intensity = clamp(intensity, 0.0, 1.0);
			// Distance attenuation
			float flashlightDistance = length(FragPos - uFlashlightPos);
			float flashlightAttenuation = 1.0 / (1.0 + 0.02 * flashlightDistance + 0.002 * flashlightDistance * flashlightDistance);
			// Use UI-configured color and brightness
			flashlightContribution = uFlashlightColor * intensity * flashlightAttenuation * uFlashlightBrightness;
		}
	}

    // Albedo sampled in linear due to sRGB texture
    vec3 texColor = texture(uAlbedoTex, vUV).rgb;
	float translucency = texture(uTranslucencyTex, vUV).r;

	if (uCullDistance > 0.0)
	{
		float dist = distance(uViewPos, FragPos);
		if (dist > uCullDistance)
			discard;
	}
	float translucencyTerm = max(0.0, dot(-lightDir, norm));
    vec3 sss = translucency * translucencyTerm * vec3(0.9, 0.95, 1.0);
    
    // UBO flashlight override
    if (uFL_Params.x > 0.5) {
        vec3 flPos = uFL_Pos.xyz;
        vec3 flDir = normalize(uFL_Dir.xyz);
        float theta = dot(flDir, normalize(FragPos - flPos));
        float inner = uFL_Params.y;
        float outer = uFL_Params.z;
        float intensity = smoothstep(outer, inner, theta);
        if (intensity > 0.0) {
            float d = length(FragPos - flPos);
            float att = 1.0 / (1.0 + 0.02 * d + 0.002 * d * d);
            vec3 Ldir = normalize(flPos - FragPos);
            float NoL = max(dot(norm, Ldir), 0.0);
            vec3 flDiffuse = NoL * uFL_Color.rgb * uFL_Params.w;
            flashlightContribution = flDiffuse * att * intensity;
        }
    }
    
    vec3 result = (ambient + diffuse + specular + flashlightContribution + sss) * (uObjectColor * texColor);
    result = max(result, vec3(0.02)); // lift absolute black slightly
    
    // Apply fog effects (unified realistic fog system - matches other shaders)
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

