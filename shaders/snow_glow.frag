#version 400 core

in vec3 vColor;
in vec3 vWorldPos;
in vec2 vLocal;

uniform int numLights;           // number of active lights (max 3)
uniform vec3 lightPos[3];        // multi-light positions
uniform vec3 lightColor[3];      // multi-light colors
uniform vec3 viewPos;
uniform float time;
uniform float glowIntensity;      // from settings
uniform float sparkleIntensity;   // from settings
uniform float sparkleThreshold;   // base threshold (0..1)
uniform float noiseScale;         // scale for hash()
uniform float tintStrength;       // 0..1 blend towards cool tint
uniform float fogStrength;        // 0..1 blend towards background color distance-based
uniform vec3 bgColor;             // background for fog
uniform float rimStrength;        // new: rim boost
uniform float rimPower;           // new: rim power
uniform float exposure;           // new: tone-map exposure
// PBR-ish material
uniform float roughness;          // 0..1 high roughness
uniform float metallic;           // 0..1 low metallic
uniform float sss;                // 0..1 translucency
uniform float anisotropy;         // 0..1 anisotropic highlight
uniform float baseAlpha;          // 0..1 transparency
uniform float edgeFade;           // exponent for edge alpha
// normals/facets
uniform float normalAmp;          // amplitude for bump
uniform float crackScale;         // scale for crack noise
uniform float crackIntensity;     // how strong crack facets are
// impostor toggle
uniform int useDisc;              // 1 for particle quads, 0 for static geometry
uniform float lodLevel;           // 0 near, 1 far – reduce expensive effects when far

out vec4 FragColor;

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec3 N;
    if (useDisc == 1)
    {
        // Circular mask and fake sphere normal from quad-local coords
        vec2 uv = vLocal; // [-1,1]
        float r2 = dot(uv, uv);
        if (r2 > 1.0) discard;
        float z = sqrt(max(0.0, 1.0 - r2));
        N = normalize(vec3(uv.xy, z));
    }
    else
    {
        // Use geometric normal with procedural bumps for static quads
        vec3 dx = dFdx(vWorldPos);
        vec3 dy = dFdy(vWorldPos);
        vec3 baseN = normalize(cross(dx, dy));
        float n1 = hash(vWorldPos.xy * crackScale);
        float n2 = hash(vWorldPos.zy * (crackScale * 1.7));
        vec3 bump = normalize(vec3(n1 - 0.5, n2 - 0.5, (n1 + n2) * 0.5)) * normalAmp;
        N = normalize(baseN + bump * crackIntensity);
        if (!gl_FrontFacing) N = -N;
    }

    vec3 V = normalize(viewPos - vWorldPos);
    float NdotV = max(dot(N, V), 0.0);
    vec3 T = normalize(cross(vec3(0.0,1.0,0.0), N));

    // Accumulate multi-light contributions
    vec3 accumDiffuse = vec3(0.0);
    vec3 accumSpecular = vec3(0.0);
    vec3 accumGlow = vec3(0.0);
    vec3 accumSubsurface = vec3(0.0);
    float maxNdotL = 0.0;
    float nearestDist = 1e9;

    int nL = clamp(numLights, 1, 3);
    for (int i = 0; i < nL; ++i)
    {
        vec3 L = normalize(lightPos[i] - vWorldPos);
        vec3 H = normalize(L + V);
        float distLight = length(lightPos[i] - vWorldPos);
        nearestDist = min(nearestDist, distLight);
        float falloff = 1.0 / (1.0 + 0.02 * distLight * distLight);

        float NdotL = max(dot(N, L), 0.0);
        maxNdotL = max(maxNdotL, NdotL);
        vec3 Ha = normalize(mix(H, normalize(T), anisotropy));
        float spec = pow(max(dot(N, Ha), 0.0), mix(16.0, 80.0, 1.0 - roughness));

        vec3 lc = lightColor[i];
        accumDiffuse += lc * NdotL * (1.0 - metallic) * (1.0 - roughness*0.5);
        accumSpecular += lc * spec * 0.7;
        float n = hash(vWorldPos.xz * noiseScale + time * 0.35 + float(i) * 12.3);
        float sparkleScale = mix(1.0, 0.3, clamp(lodLevel, 0.0, 1.0));
        float sparkle = smoothstep(sparkleThreshold, 1.0, n + NdotV * 0.25) * sparkleIntensity * sparkleScale;
        accumGlow += lc * sparkle * glowIntensity * falloff;
        float NdotLback = max(dot(-N, L), 0.0);
        accumSubsurface += lc * NdotLback * sss * 0.6;
    }

    float rim = pow(1.0 - NdotV, rimPower) * rimStrength;
    vec3 base = vColor * (0.6 + 0.4 * maxNdotL);
    vec3 diffuse = accumDiffuse;
    vec3 specular = accumSpecular;
    vec3 glow = accumGlow;
    vec3 rimCol = vec3(0.7, 0.85, 1.0) * rim;

    vec3 subsurface = accumSubsurface;

    float normalScale = mix(1.0, 0.2, clamp(lodLevel, 0.0, 1.0));
    vec3 col = base + diffuse + specular + glow + rimCol + subsurface;
    col = mix(col, vec3(0.8, 0.9, 1.0), tintStrength);

    float fog = clamp(fogStrength * (nearestDist / 120.0), 0.0, 1.0);
    col = mix(col, bgColor, fog);

    col = vec3(1.0) - exp(-col * exposure);
    float alpha = baseAlpha * pow(NdotV, edgeFade);
    FragColor = vec4(col, alpha);
}


