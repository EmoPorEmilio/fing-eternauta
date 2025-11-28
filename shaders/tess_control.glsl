// Tessellation Control Shader: passes through and sets tess levels
#version 450 core
layout(vertices = 3) out;

in vec3 FragPos[];
in vec3 Normal[];
in vec2 vUV[];

out vec3 tc_FragPos[];
out vec3 tc_Normal[];
out vec2 tc_UV[];

uniform float uTessLevelInner = 8.0;
uniform float uTessLevelOuter = 8.0;

void main()
{
    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = uTessLevelInner;
        gl_TessLevelOuter[0] = uTessLevelOuter;
        gl_TessLevelOuter[1] = uTessLevelOuter;
        gl_TessLevelOuter[2] = uTessLevelOuter;
        gl_TessLevelOuter[3] = uTessLevelOuter;
    }
    tc_FragPos[gl_InvocationID] = FragPos[gl_InvocationID];
    tc_Normal[gl_InvocationID] = Normal[gl_InvocationID];
    tc_UV[gl_InvocationID] = vUV[gl_InvocationID];
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}

