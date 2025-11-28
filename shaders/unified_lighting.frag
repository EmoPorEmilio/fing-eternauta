#version 450 core

in vec3 FragPos;
in vec3 Normal;
in vec2 vUV;

out vec4 FragColor;

// Object properties
uniform vec3 uObjectColor;
uniform sampler2D uTex;
uniform bool uUseTexture;

// Camera
uniform vec3 uViewPos;

// Lighting system
uniform int uNumLights;
struct Light {
    int type;           // 0=DIRECTIONAL, 1=POINT, 2=SPOTLIGHT
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float cutoff;
    float outerCutoff;
    float constant;
    float linear;
    float quadratic;
    int enabled;
};

uniform Light uLights[16]; // Support up to 16 lights

// Distance culling
uniform float uCullDistance;

// Lighting calculation functions
vec3 calculateDirectionalLight(Light light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    vec3 diffuse = diff * light.color * light.intensity;
    vec3 specular = spec * light.color * light.intensity * 0.4;
    
    return diffuse + specular;
}

vec3 calculatePointLight(Light light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - FragPos);
    float distance = length(light.position - FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    
    vec3 diffuse = diff * light.color * light.intensity * attenuation;
    vec3 specular = spec * light.color * light.intensity * attenuation * 0.4;
    
    return diffuse + specular;
}

vec3 calculateSpotlight(Light light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - FragPos);
    float theta = dot(lightDir, normalize(-light.direction));
    
    if (theta > light.cutoff) {
        float epsilon = light.cutoff - light.outerCutoff;
        float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);
        
        float distance = length(light.position - FragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
        
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
        
        vec3 diffuse = diff * light.color * light.intensity * intensity * attenuation;
        vec3 specular = spec * light.color * light.intensity * intensity * attenuation * 0.4;
        
        return diffuse + specular;
    }
    
    return vec3(0.0);
}

void main() {
    // Distance-based culling
    if (uCullDistance > 0.0) {
        float dist = distance(uViewPos, FragPos);
        if (dist > uCullDistance)
            discard;
    }
    
    // Ambient lighting
    vec3 ambient = vec3(0.02);
    
    // Surface properties
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uViewPos - FragPos);
    
    // Calculate lighting from all lights
    vec3 totalLighting = ambient;
    
    for (int i = 0; i < uNumLights; i++) {
        if (uLights[i].enabled == 1) {
            if (uLights[i].type == 0) { // DIRECTIONAL
                totalLighting += calculateDirectionalLight(uLights[i], norm, viewDir);
            } else if (uLights[i].type == 1) { // POINT
                totalLighting += calculatePointLight(uLights[i], norm, viewDir);
            } else if (uLights[i].type == 2) { // SPOTLIGHT
                totalLighting += calculateSpotlight(uLights[i], norm, viewDir);
            }
        }
    }
    
    // Apply texture if needed
    vec3 finalColor = uObjectColor;
    if (uUseTexture) {
        vec3 texColor = texture(uTex, vUV).rgb;
        finalColor = mix(uObjectColor, uObjectColor * texColor, 0.1);
    }
    
    FragColor = vec4(totalLighting * finalColor, 1.0);
}
