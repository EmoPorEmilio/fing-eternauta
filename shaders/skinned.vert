#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aJoints;
layout (location = 4) in vec4 aWeights;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uBones[128];
uniform int uUseSkinning;

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vFragPos;
out vec4 vFragPosLightSpace;

void main()
{
    vec4 skinnedPos;
    vec3 skinnedNormal;

    if (uUseSkinning == 1)
    {
        mat4 skinMatrix =
            aWeights.x * uBones[int(aJoints.x)] +
            aWeights.y * uBones[int(aJoints.y)] +
            aWeights.z * uBones[int(aJoints.z)] +
            aWeights.w * uBones[int(aJoints.w)];

        skinnedPos = skinMatrix * vec4(aPos, 1.0);
        skinnedNormal = mat3(skinMatrix) * aNormal;
    }
    else
    {
        skinnedPos = vec4(aPos, 1.0);
        skinnedNormal = aNormal;
    }

    vec4 worldPos = uModel * skinnedPos;
    vFragPos = worldPos.xyz;
    vFragPosLightSpace = uLightSpaceMatrix * worldPos;
    vNormal = mat3(transpose(inverse(uModel))) * skinnedNormal;
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * worldPos;
}
