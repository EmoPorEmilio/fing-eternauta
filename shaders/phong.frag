#version 450 core
// Expect FragPos/Normal/vUV from TES
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
uniform sampler2D uHeightTex;
uniform bool uDebugDisp;
uniform bool uDebugNormals;
uniform bool uDebugGrad;
uniform float uCullDistance; // if > 0, discard fragments farther than this from view pos
uniform bool uFlashlightOn;
uniform vec3 uFlashlightPos;
uniform vec3 uFlashlightDir;
uniform float uFlashlightCutoff;

void main()
{
	// Ambient
	vec3 ambient = 0.1 * uLightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = diff * uLightColor;

    // Specular (roughness-aware)
    float specularStrength = 0.4;
    vec3 viewDir = normalize(uViewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float roughness = texture(uRoughnessTex, vUV).r;
    float shininess = mix(8.0, 256.0, pow(1.0 - clamp(roughness, 0.0, 1.0), 2.0));
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * uLightColor;

	// Flashlight lighting
	vec3 flashlightContribution = vec3(0.0);
	if (uFlashlightOn) {
		vec3 flashlightDir = normalize(uFlashlightDir);
		vec3 lightToFrag = normalize(FragPos - uFlashlightPos);
		float theta = dot(lightToFrag, flashlightDir);
		
		if (theta > uFlashlightCutoff) {
			float intensity = (theta - uFlashlightCutoff) / (1.0 - uFlashlightCutoff);
			intensity = clamp(intensity, 0.0, 1.0);
			flashlightContribution = vec3(1.0, 0.8, 0.6) * intensity * 3.0;
		}
	}

    vec2 uv = vUV;
    vec3 texColor = texture(uAlbedoTex, uv).rgb;
    float heightVal = texture(uHeightTex, uv).r;
    float translucency = texture(uTranslucencyTex, uv).r;

	// Optional distance-based fade/cull to avoid far z-fighting/overdraw
	if (uCullDistance > 0.0)
	{
        float dist = distance(uViewPos, FragPos);
		if (dist > uCullDistance)
			discard;
	}
    float translucencyTerm = max(0.0, dot(-lightDir, norm));
    vec3 sss = translucency * translucencyTerm * vec3(0.9, 0.95, 1.0);
    vec3 result = (ambient + diffuse + specular + flashlightContribution + sss) * (uObjectColor * texColor);
    if (uDebugDisp) {
        // Visualize displacement height as grayscale overlay
        vec3 heightVis = vec3(heightVal);
        result = mix(result, heightVis, 0.6);
    }
    if (uDebugNormals) {
        vec3 nvis = normalize(Normal) * 0.5 + 0.5;
        // Mark with blue tint if normals are nearly flat
        float var = length(dFdx(nvis) + dFdy(nvis));
        if (var < 0.02) nvis = mix(nvis, vec3(0.2, 0.2, 1.0), 0.6);
        result = nvis;
    }
    if (uDebugGrad) {
        ivec2 ts = textureSize(uHeightTex, 0);
        vec2 texel = 1.0 / vec2(ts);
        float h = texture(uHeightTex, uv).r;
        float hx = texture(uHeightTex, uv + vec2(texel.x, 0.0)).r;
        float hy = texture(uHeightTex, uv + vec2(0.0, texel.y)).r;
        float gx = abs(hx - h);
        float gy = abs(hy - h);
        result = vec3(gx, gy, 0.0) * 8.0; // amplify to visualize
    }
	FragColor = vec4(result, 1.0);
}
