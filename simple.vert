#version 400 core
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;


layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Color;

out vec3 vertex_color;

void main(void){
    mat4 MVP = projection * view* model;
	gl_Position = MVP * vec4(in_Position,1.0);
	vertex_color = in_Color;
}