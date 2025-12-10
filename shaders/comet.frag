#version 450 core

in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vFragPos;
in vec3 vLocalPos;
in float vTrailFade;    // 0 at comet body, 1 at trail end
flat in vec3 vCometCenter;   // World position of comet center (flat = no interpolation)
flat in float vTrailLength;  // Total trail length for normalization (flat = no interpolation)

out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uHasTexture;      // 1 = use texture, 0 = use solid color
uniform vec3 uCometColor;     // Fallback color if no texture
uniform int uDebugMode;       // 1 = color by axis for orientation testing

void main()
{
    if (uDebugMode == 1) {
        // Debug: color by local axis position
        // Red = +X, Green = +Y, Blue = +Z (negative shows darker)
        vec3 axisColor = vec3(0.5) + vLocalPos * 0.5;  // Map -1..1 to 0..1
        FragColor = vec4(axisColor, 1.0);
    } else {
        vec3 baseColor;
        float baseAlpha = 1.0;

        // Sample texture or use solid color
        if (uHasTexture == 1) {
            vec4 texColor = texture(uTexture, vTexCoord);
            baseColor = texColor.rgb;
            baseAlpha = texColor.a;
        } else {
            baseColor = uCometColor;
        }

        // Opacity fade:
        // vTrailFade 0.0 -> 0.1: opacity 100% -> 70% (very head)
        // vTrailFade 0.1 -> 0.7: opacity 70% -> 30% (most of the trail - stretched mid-range)
        // vTrailFade 0.7 -> 1.0: opacity 30% -> 0% (tail end)
        float trailAlpha;
        if (vTrailFade < 0.1) {
            trailAlpha = mix(1.0, 0.7, vTrailFade / 0.1);
        } else if (vTrailFade < 0.7) {
            trailAlpha = mix(0.7, 0.3, (vTrailFade - 0.1) / 0.6);
        } else {
            trailAlpha = mix(0.3, 0.0, (vTrailFade - 0.7) / 0.3);
        }

        FragColor = vec4(baseColor, baseAlpha * trailAlpha);
    }
}
