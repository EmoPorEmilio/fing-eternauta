#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
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
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vFragPosLightSpace = uLightSpaceMatrix * worldPos;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vWorldNormal = normalize(vNormal);  // Normalized world-space normal
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * worldPos;
}
