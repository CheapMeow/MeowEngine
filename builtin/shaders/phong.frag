#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform PointLight{
	vec3 pos;
	vec3 viewPos;
    int blinn;
} light;

layout (set = 2, binding = 0) uniform sampler2D diffuseMap;

void main() 
{
    vec3 color = texture(diffuseMap, inUV0).rgb;
    // Ambient
    vec3 ambient = 0.05 * color;
    // Diffuse
    vec3 lightDir = normalize(light.pos - inPosition);
    vec3 normal = normalize(inNormal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // Specular
    vec3 viewDir = normalize(light.viewPos - inPosition);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    if(light.blinn == 0)
    {
        vec3 halfwayDir = normalize(lightDir + viewDir);  
        spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    }
    else
    {
        vec3 reflectDir = reflect(-lightDir, normal);
        spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
    }
    vec3 specular = vec3(0.3) * spec; // assuming bright white light color
    outFragColor = vec4(ambient + diffuse + specular, 1.0f);
}