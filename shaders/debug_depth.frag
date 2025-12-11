#version 450 core
out vec4 FragColor;
in vec2 vTexCoord;

uniform sampler2D uDepthMap;

void main() {
    float depth = texture(uDepthMap, vTexCoord).r;
    // Visualize depth - near is black, far is white
    // Enhance contrast to see detail better
    FragColor = vec4(vec3(depth), 1.0);
}
