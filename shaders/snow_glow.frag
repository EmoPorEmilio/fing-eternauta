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
uniform float mixAmount;          // new: mix SnowGlow with a simpler lambert/spec
// Additional depth/height fog controls
uniform float depthDesatStrength; // distance-based desaturation strength
uniform float depthBlueStrength;  // distance-based blue shift strength
uniform float fogHeightStrength;  // additional fog contribution by height
// Accumulation and jitter controls
uniform float accumulationStrength;    // static surface snow blend
uniform float accumulationCoverage;    // threshold for coverage noise
uniform float accumulationNoiseScale;  // tiling for coverage noise
uniform vec3 accumulationColor;        // accumulated snow color
uniform float jitterIntensity;         // temporal jitter amplitude for sparkle
uniform float jitterSpeed;             // temporal jitter speed
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

// Needed for correct impostor depth
uniform mat4 projection;
uniform mat4 view;
uniform vec3 billboardCenter;
uniform float spriteSize;

out vec4 FragColor;

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main() {
    vec3 N;
    float r2Local = 0.0;
    vec3 P = vWorldPos; // world-space position used for lighting (sphere surface for discs)
    // Treat any fragment from billboarded impostors (nonzero vLocal) as disc-masked,
    // even if the uniform was not set correctly.
    if (useDisc == 1 || dot(vLocal, vLocal) > 0.0)
    {
        // Circular mask and fake sphere normal from quad-local coords
        vec2 uv = vLocal; // [-1,1]
        float r2 = dot(uv, uv);
        r2Local = r2;
        if (r2 > 1.0) discard;
        float z = sqrt(max(0.0, 1.0 - r2));
        N = normalize(vec3(uv.xy, z));

        // Correct depth and lighting position on impostor sphere surface
        vec3 sphereWorldPos = billboardCenter + N * spriteSize;
        vec4 clipPos = projection * view * vec4(sphereWorldPos, 1.0);
        float ndcDepth = clipPos.z / clipPos.w;         // -1..1
        gl_FragDepth = ndcDepth * 0.5 + 0.5;            // 0..1
        P = sphereWorldPos;
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

    vec3 V = normalize(viewPos - P);
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
        vec3 L = normalize(lightPos[i] - P);
        vec3 H = normalize(L + V);
        float distLight = length(lightPos[i] - P);
        nearestDist = min(nearestDist, distLight);
        float falloff = 1.0 / (1.0 + 0.02 * distLight * distLight);

        float NdotL = max(dot(N, L), 0.0);
        maxNdotL = max(maxNdotL, NdotL);
        vec3 Ha = normalize(mix(H, normalize(T), anisotropy));
        // Slightly stronger spec to avoid flatness on small sprites
        float spec = pow(max(dot(N, Ha), 0.0), mix(24.0, 96.0, 1.0 - roughness));

        vec3 lc = lightColor[i];
        accumDiffuse += lc * NdotL * (1.0 - metallic) * (1.0 - roughness*0.5);
        accumSpecular += lc * spec * 0.7;
        float n = hash(P.xz * noiseScale + time * 0.35 + float(i) * 12.3);
        float jitter = sin(time * jitterSpeed + hash(P.xy * 13.37) * 6.28318) * jitterIntensity;
        float sparkleScale = mix(1.0, 0.3, clamp(lodLevel, 0.0, 1.0));
        float sparkle = smoothstep(sparkleThreshold, 1.0, n + jitter + NdotV * 0.25) * sparkleIntensity * sparkleScale;
        accumGlow += lc * sparkle * glowIntensity * falloff;
        float NdotLback = max(dot(-N, L), 0.0);
        accumSubsurface += lc * NdotLback * sss * 0.6;
    }

    float rim = pow(1.0 - NdotV, rimPower) * rimStrength * 1.2;
    // Cooler base; slight blue bias
    vec3 base = mix(vColor, vec3(0.8, 0.9, 1.0), 0.2) * (0.55 + 0.45 * maxNdotL);
    // Sphere AO from radius (disc) or view fresnel fallback
    float ao = 1.0 - smoothstep(0.65, 1.0, r2Local);
    if (useDisc == 0) ao = 1.0 - pow(1.0 - NdotV, 2.0) * 0.35;
    vec3 diffuse = accumDiffuse * (0.8 + 0.2 * ao);
    vec3 specular = accumSpecular;
    vec3 glow = accumGlow;
    vec3 rimCol = vec3(0.7, 0.85, 1.0) * rim;

    vec3 subsurface = accumSubsurface;

    float normalScale = mix(1.0, 0.2, clamp(lodLevel, 0.0, 1.0));
    // Fresnel/center ramp to emphasize spherical volume
    float center = NdotV; // higher at center of sphere
    vec3 centerTint = vec3(0.9, 0.95, 1.0) * pow(center, 2.0) * 0.3;
    // Center blue to emphasize spherical volume (stronger near disc center)
    float centerFactor = (useDisc == 1) ? (1.0 - smoothstep(0.0, 0.8, sqrt(r2Local))) : pow(NdotV, 2.0);
    vec3 centerBlue = vec3(0.82, 0.9, 1.0) * centerFactor * 0.35;
    vec3 snowCol = base + diffuse + specular + glow + rimCol + subsurface + centerTint + centerBlue;

    // Ground accumulation only for static surfaces (not discs)
    if (useDisc == 0)
    {
        // Noise-based coverage, stronger on upward-facing surfaces
        float up = clamp(dot(N, vec3(0.0, 1.0, 0.0)), 0.0, 1.0);
        float covNoise = hash(vWorldPos.xz * accumulationNoiseScale);
        float coverage = smoothstep(accumulationCoverage - 0.2, accumulationCoverage + 0.2, covNoise * (0.5 + 0.5 * up));
        vec3 accCol = mix(snowCol, accumulationColor, coverage * accumulationStrength * up);
        snowCol = accCol;
    }

    // Aesthetic tweaks: tint and fog
    vec3 col = mix(snowCol, vec3(0.8, 0.9, 1.0), clamp(tintStrength, 0.0, 1.0));
    float fog = clamp(fogStrength * (nearestDist / 160.0), 0.0, 0.8);
    fog += clamp(fogHeightStrength * max(0.0, P.y) / 120.0, 0.0, 0.25);
    col = mix(col, bgColor, clamp(fog, 0.0, 0.8));

    // Distance-based desaturation and blue shift (gentler)
    float dNorm = clamp(nearestDist / 140.0, 0.0, 1.0);
    float desat = depthDesatStrength * dNorm * 0.5;
    float luma = dot(col, vec3(0.299, 0.587, 0.114));
    col = mix(col, vec3(luma), desat);
    vec3 coldBlue = vec3(0.78, 0.86, 1.0);
    float blueAmt = depthBlueStrength * dNorm * 0.4;
    col = mix(col, coldBlue, blueAmt);

    // Simple alternate lambert/spec for mixing
    vec3 altLambert = vColor * (0.5 + 0.5 * maxNdotL) + accumDiffuse * 0.8;
    vec3 altSpec = accumSpecular * 0.4;
    vec3 altCol = altLambert + altSpec;

    // Blend with alternate model, then apply exposure/tone mapping
    vec3 colMixed = mix(col, altCol, clamp(mixAmount, 0.0, 1.0));
    col = vec3(1.0) - exp(-colMixed * max(exposure, 1e-4));

    // Radial gradient fallback to ensure visible sphericity on impostors
    float radial = (useDisc == 1 || r2Local > 0.0) ? clamp(1.0 - r2Local, 0.0, 1.0) : NdotV;
    col = mix(col * 0.7, col, radial);

    float alpha = baseAlpha * pow(NdotV, edgeFade);
    FragColor = vec4(col, alpha);
}


