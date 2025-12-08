#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 uProjection;
uniform vec2 uPosition;
uniform vec2 uSize;

void main() {
    vec2 pos = aPos * uSize + uPosition;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
