// Tessellation Evaluation Shader: displaces along normal using height map
#version 450 core
layout(triangles, equal_spacing, cw) in;

in vec3 tc_FragPos[];
in vec3 tc_Normal[];
in vec2 tc_UV[];

out vec3 FragPos;
out vec3 Normal;
out vec2 vUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

uniform sampler2D uHeightTex; // snow_02_disp_1k.png
uniform float uHeightScale = 0.05;
uniform vec2 uWorldPerUV = vec2(10.0, 10.0); // world units per 1.0 UV step along (u,v)
uniform float uNormalAmplify = 1.0; // debug scaling for normal perturbation

void main()
{
    vec3 p0 = tc_FragPos[0];
    vec3 p1 = tc_FragPos[1];
    vec3 p2 = tc_FragPos[2];
    vec2 uv0 = tc_UV[0];
    vec2 uv1 = tc_UV[1];
    vec2 uv2 = tc_UV[2];
    vec3 n0 = normalize(tc_Normal[0]);
    vec3 n1 = normalize(tc_Normal[1]);
    vec3 n2 = normalize(tc_Normal[2]);

    vec3 bary = vec3(gl_TessCoord.x, gl_TessCoord.y, gl_TessCoord.z);
    vec3 p = p0 * bary.x + p1 * bary.y + p2 * bary.z;
    vec2 uv = uv0 * bary.x + uv1 * bary.y + uv2 * bary.z;
    vec3 n = normalize(n0 * bary.x + n1 * bary.y + n2 * bary.z);

    // Enable displacement
    float h = texture(uHeightTex, uv).r; // [0,1]
    float displacement = (h - 0.5) * 2.0 * uHeightScale; // center around 0
    vec3 displaced = p + n * displacement;

    // Recompute normal using height gradients in a fixed tangent frame (floor plane)
    // Approximate world-space tangent and bitangent for the floor
    vec3 T = normalize((uModel * vec4(1.0, 0.0, 0.0, 0.0)).xyz);
    vec3 B = normalize((uModel * vec4(0.0, 0.0, 1.0, 0.0)).xyz);
    ivec2 texSize = textureSize(uHeightTex, 0);
    vec2 texel = 1.0 / vec2(texSize);
    float h_u = texture(uHeightTex, uv + vec2(texel.x, 0.0)).r;
    float h_v = texture(uHeightTex, uv + vec2(0.0, texel.y)).r;
    // Convert height to signed range and compute derivatives per UV unit
    float H = (h - 0.5) * 2.0;
    float H_u = (h_u - 0.5) * 2.0;
    float H_v = (h_v - 0.5) * 2.0;
    float dHdu = (H_u - H) / texel.x; // per 1.0 UV
    float dHdv = (H_v - H) / texel.y; // per 1.0 UV
    // Map UV derivatives to world using tangent frame scale
    vec3 bump = (dHdu / max(uWorldPerUV.x, 1e-4)) * T + (dHdv / max(uWorldPerUV.y, 1e-4)) * B;
    vec3 n_recomputed = normalize(n - (uHeightScale * uNormalAmplify) * bump);

    FragPos = displaced;
    Normal = n_recomputed;
    vUV = uv;
    gl_Position = uProj * uView * vec4(displaced, 1.0);
}

