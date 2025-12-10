#version 450 core
layout (location = 0) in vec2 aPos;

out vec2 vTexCoord;

void main()
{
    // Fullscreen quad: aPos is already in NDC (-1 to 1)
    gl_Position = vec4(aPos, 0.0, 1.0);
    // Convert from NDC to UV (0 to 1)
    vTexCoord = aPos * 0.5 + 0.5;
}
