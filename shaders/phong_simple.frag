#version 400 core

in vec3 vertex_color;
in vec3 worldPos;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

out vec4 FragColor;

void main(void) {
    // Simple lighting without normals - use position-based approximation
    vec3 lightDir = normalize(lightPos - worldPos);
    vec3 viewDir = normalize(viewPos - worldPos);
    
    // Ambient lighting
    vec3 ambient = ambientStrength * lightColor;
    
    // Simple diffuse (without normal)
    float diff = max(dot(normalize(worldPos), lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;
    
    // Simple specular (without normal)
    vec3 reflectDir = reflect(-lightDir, normalize(worldPos));
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Combine lighting
    vec3 result = (ambient + diffuse + specular) * vertex_color;
    FragColor = vec4(result, 1.0);
}
