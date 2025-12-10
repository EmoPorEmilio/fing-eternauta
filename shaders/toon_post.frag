#version 450 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uSceneTex;
uniform vec2 uTexelSize;  // 1.0 / resolution

// Convert to grayscale using luminance
float luminance(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

// Sobel edge detection
float sobel()
{
    // Sample 3x3 neighborhood
    float tl = luminance(texture(uSceneTex, vTexCoord + vec2(-1, -1) * uTexelSize).rgb);
    float t  = luminance(texture(uSceneTex, vTexCoord + vec2( 0, -1) * uTexelSize).rgb);
    float tr = luminance(texture(uSceneTex, vTexCoord + vec2( 1, -1) * uTexelSize).rgb);
    float l  = luminance(texture(uSceneTex, vTexCoord + vec2(-1,  0) * uTexelSize).rgb);
    float r  = luminance(texture(uSceneTex, vTexCoord + vec2( 1,  0) * uTexelSize).rgb);
    float bl = luminance(texture(uSceneTex, vTexCoord + vec2(-1,  1) * uTexelSize).rgb);
    float b  = luminance(texture(uSceneTex, vTexCoord + vec2( 0,  1) * uTexelSize).rgb);
    float br = luminance(texture(uSceneTex, vTexCoord + vec2( 1,  1) * uTexelSize).rgb);

    // Sobel kernels
    float gx = -tl - 2.0*l - bl + tr + 2.0*r + br;
    float gy = -tl - 2.0*t - tr + bl + 2.0*b + br;

    return sqrt(gx*gx + gy*gy);
}

void main()
{
    vec3 color = texture(uSceneTex, vTexCoord).rgb;
    float lum = luminance(color);

    // Edge detection
    float edge = sobel();

    // Threshold edge for crisp ink lines - only strong edges become ink
    float inkLine = smoothstep(0.15, 0.4, edge);

    // Comic book style: mostly white paper
    // Very low threshold means almost everything becomes white
    // Only the darkest areas (shadows) stay gray/dark
    float paper = smoothstep(0.02, 0.15, lum);

    // Boost toward white even more - comic books are bright
    paper = mix(paper, 1.0, 0.7);

    // Final: white paper with black ink strokes
    // Ink lines are always black, paper is mostly white
    float result = paper * (1.0 - inkLine);

    FragColor = vec4(vec3(result), 1.0);
}
