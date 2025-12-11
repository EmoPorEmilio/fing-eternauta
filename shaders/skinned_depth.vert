#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;      // Not used but required for VAO compatibility
layout (location = 2) in vec2 aTexCoord;    // Not used but required for VAO compatibility
layout (location = 3) in vec4 aJoints;
layout (location = 4) in vec4 aWeights;

uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uBones[128];
uniform int uUseSkinning;

void main() {
    vec4 skinnedPos;

    if (uUseSkinning == 1) {
        mat4 skinMatrix =
            aWeights.x * uBones[int(aJoints.x)] +
            aWeights.y * uBones[int(aJoints.y)] +
            aWeights.z * uBones[int(aJoints.z)] +
            aWeights.w * uBones[int(aJoints.w)];

        skinnedPos = skinMatrix * vec4(aPos, 1.0);
    } else {
        skinnedPos = vec4(aPos, 1.0);
    }

    gl_Position = uLightSpaceMatrix * uModel * skinnedPos;
}
