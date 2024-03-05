#version 450

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

layout (binding = 0) uniform sampler2D diffuseMap;

void main() 
{
    vec3 normal = normalize(inNormal);
    vec3 lightDir = vec3(0, 0, -1);
    float diffuse = dot(normal, lightDir);
    outFragColor = vec4(diffuse * inColor * texture(diffuseMap, inUV).xyz, 1.0);
}