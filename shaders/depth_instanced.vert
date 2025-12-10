#version 450 core
layout (location = 0) in vec3 aPos;
// Instanced model matrix (takes 4 attribute slots for mat4)
layout (location = 5) in mat4 aInstanceModel;

uniform mat4 uLightSpaceMatrix;

void main() {
    gl_Position = uLightSpaceMatrix * aInstanceModel * vec4(aPos, 1.0);
}
