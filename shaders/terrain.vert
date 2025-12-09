#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uTexScale;  // How many times texture repeats per unit

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vFragPos;
out vec3 vColor;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;

    // World-space UV mapping - texture tiles based on XZ position
    vTexCoord = worldPos.xz * uTexScale;

    // Pass vertex color (used for shadow/darkness in deformations)
    vColor = aColor;

    gl_Position = uProjection * uView * worldPos;
}
