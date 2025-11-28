#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

// Also feed TES pipeline
out vec3 FragPos;
out vec3 Normal;
out vec2 vUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
	vec4 worldPos = uModel * vec4(aPos, 1.0);
	FragPos = worldPos.xyz;
	Normal = mat3(transpose(inverse(uModel))) * aNormal;
	vUV = aUV;
	gl_Position = uProj * uView * worldPos;
}
