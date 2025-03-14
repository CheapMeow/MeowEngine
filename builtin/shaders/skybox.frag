#version 450

layout(set = 0, binding = 0) uniform samplerCube environmentMap;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outFragColor;

void main()
{
    vec3 envColor = texture(environmentMap, inPosition).rgb;

    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0 / 2.2));

    outFragColor = vec4(envColor, 1.0);
}