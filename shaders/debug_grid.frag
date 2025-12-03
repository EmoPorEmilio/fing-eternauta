#version 450 core

// Blender-style infinite grid fragment shader
// Computes grid pattern procedurally with distance fade

in vec3 nearPoint;
in vec3 farPoint;
in mat4 fragView;
in mat4 fragProj;

out vec4 fragColor;

uniform float uGridScale;      // Grid cell size (1.0 = 1 meter)
uniform float uFadeDistance;   // Distance at which grid starts fading
uniform vec3 uAxisColorX;      // X axis color (red)
uniform vec3 uAxisColorZ;      // Z axis color (blue)
uniform vec3 uGridColorMinor;  // Minor grid line color
uniform vec3 uGridColorMajor;  // Major grid line color

// Compute grid pattern for given scale
vec4 grid(vec3 fragPos3D, float scale, vec3 lineColor) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float alpha = 1.0 - min(line, 1.0);

    vec4 color = vec4(lineColor, alpha);

    // Axis highlighting
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);

    // X axis (red line along Z=0)
    if (fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz) {
        color.rgb = uAxisColorX;
        color.a = max(color.a, 0.8);
    }

    // Z axis (blue line along X=0)
    if (fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx) {
        color.rgb = uAxisColorZ;
        color.a = max(color.a, 0.8);
    }

    return color;
}

// Compute depth for proper depth buffer interaction
float computeDepth(vec3 pos) {
    vec4 clipSpace = fragProj * fragView * vec4(pos, 1.0);
    float depth = clipSpace.z / clipSpace.w;
    return depth * 0.5 + 0.5;
}

// Compute linear depth for fade calculation
float computeLinearDepth(vec3 pos) {
    vec4 clipSpace = fragProj * fragView * vec4(pos, 1.0);
    float clipDepth = clipSpace.z / clipSpace.w;
    float near = 0.1;
    float far = 2000.0;
    float linearDepth = (2.0 * near * far) / (far + near - clipDepth * (far - near));
    return linearDepth / far;
}

void main() {
    // Ray-plane intersection: find t where ray hits Y=0 plane
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);

    // Discard if plane is behind camera
    if (t < 0.0) {
        discard;
    }

    // Calculate 3D position on the Y=0 plane
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

    // Write proper depth
    gl_FragDepth = computeDepth(fragPos3D);

    // Calculate distance-based fade
    float linearDepth = computeLinearDepth(fragPos3D);
    float fadeFactor = max(0.0, 1.0 - linearDepth * 2.0);

    // Additional fade based on configurable distance
    float distFromCamera = length(fragPos3D - nearPoint);
    float distFade = 1.0 - smoothstep(uFadeDistance * 0.5, uFadeDistance, distFromCamera);

    // Combine both fade factors
    float fading = fadeFactor * distFade;

    // Render two grid levels: minor (1 unit) and major (10 units)
    vec4 minorGrid = grid(fragPos3D, uGridScale, uGridColorMinor);
    vec4 majorGrid = grid(fragPos3D, uGridScale * 0.1, uGridColorMajor);

    // Blend grids - major grid takes precedence
    fragColor = minorGrid;
    fragColor.rgb = mix(fragColor.rgb, majorGrid.rgb, majorGrid.a * 0.8);
    fragColor.a = max(minorGrid.a, majorGrid.a);

    // Apply distance fade
    fragColor.a *= fading;

    // Discard fully transparent fragments
    if (fragColor.a < 0.001) {
        discard;
    }
}
