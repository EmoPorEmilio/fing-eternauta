#version 450 core

// Blender-style infinite grid shader
// Renders a procedural grid on the Y=0 plane using ray-casting

out vec3 nearPoint;
out vec3 farPoint;
out mat4 fragView;
out mat4 fragProj;

uniform mat4 uView;
uniform mat4 uProj;

// Fullscreen quad vertices (two triangles)
vec3 gridPlane[6] = vec3[](
    vec3( 1,  1, 0), vec3(-1, -1, 0), vec3(-1,  1, 0),
    vec3(-1, -1, 0), vec3( 1,  1, 0), vec3( 1, -1, 0)
);

// Unproject screen point to world space
vec3 unprojectPoint(float x, float y, float z, mat4 view, mat4 proj) {
    mat4 viewInv = inverse(view);
    mat4 projInv = inverse(proj);
    vec4 unprojected = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojected.xyz / unprojected.w;
}

void main() {
    vec3 p = gridPlane[gl_VertexID];

    // Unproject near and far points for ray-casting
    nearPoint = unprojectPoint(p.x, p.y, 0.0, uView, uProj);
    farPoint = unprojectPoint(p.x, p.y, 1.0, uView, uProj);

    // Pass matrices to fragment shader
    fragView = uView;
    fragProj = uProj;

    gl_Position = vec4(p, 1.0);
}
