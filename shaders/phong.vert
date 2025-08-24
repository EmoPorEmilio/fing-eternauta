#version 400 core

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat3 normalMatrix;

layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Color;
layout (location = 2) in vec3 in_Normal;

out vec3 vertex_color;
out vec3 worldPos;
out vec3 worldNormal;

void main(void) {
    mat4 MVP = projection * view * model;
    gl_Position = MVP * vec4(in_Position, 1.0);
    
    worldPos = vec3(model * vec4(in_Position, 1.0));
    worldNormal = normalMatrix * in_Normal;
    vertex_color = in_Color;
}
