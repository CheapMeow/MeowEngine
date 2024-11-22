#version 450

struct PointLight{
	vec3 pos;
	vec3 viewPos;
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inUV0;
layout (location = 2) flat in PointLight inLight;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 1) uniform sampler2D diffuseMap;
layout (set = 1, binding = 2) uniform sampler2D normalMap;

void main() 
{
    // obtain normal from normal map in range [0,1]
    vec3 normal = texture(normalMap, inUV0).rgb;
    // transform normal vector to range [-1,1]
    normal = normalize(normal * 2.0 - 1.0);  // this normal is in tangent space
   
    // get diffuse color
    vec3 color = texture(diffuseMap, inUV0).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 lightDir = normalize(inLight.pos - inPosition);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular
    vec3 viewDir = normalize(inLight.viewPos - inPosition);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    outFragColor = vec4(ambient + diffuse + specular, 1.0);
}