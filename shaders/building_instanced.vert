#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
// Instanced model matrix (takes 4 attribute slots for mat4)
layout (location = 5) in mat4 aInstanceModel;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vFragPos;
out vec4 vFragPosLightSpace;
out vec3 vWorldNormal;  // For triplanar mapping

void main()
{
    // Use instance model matrix instead of uniform
    mat4 model = aInstanceModel;
    vec4 worldPos = model * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vFragPosLightSpace = uLightSpaceMatrix * worldPos;
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vWorldNormal = normalize(vNormal);
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * worldPos;
}
