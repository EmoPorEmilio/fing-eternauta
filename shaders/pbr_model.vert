#version 330 core

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec3 in_Color;
layout (location = 3) in vec2 in_TexCoord;
layout (location = 4) in vec4 in_Tangent;
layout (location = 5) in vec4 in_Weights;
layout (location = 6) in uvec4 in_Joints;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
uniform bool uSkinned;
uniform int  uJointCount;
uniform mat4 uJointMatrices[64];

out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
out vec2 vUV;

void main()
{
    vec4 pos = vec4(in_Position, 1.0);
    vec3 normal = in_Normal;
    if (uSkinned) {
        // Linear blend skinning with up to 4 weights
        vec4 blended = vec4(0.0);
        mat3 nblend = mat3(0.0);
        for (int i = 0; i < 4; ++i) {
            int j = int(in_Joints[i]);
            float w = in_Weights[i];
            if (w > 0.0 && j < uJointCount) {
                mat4 J = uJointMatrices[j];
                blended += J * pos * w;
                nblend += mat3(J) * w;
            }
        }
        pos = blended;
        normal = normalize(nblend * normal);
    }
    vec4 world = uModel * pos;
    vWorldPos = world.xyz;
    vNormal = normalize(uNormalMatrix * normal);
    vColor = in_Color;
    vUV = in_TexCoord;
    gl_Position = uProjection * uView * world;
}
