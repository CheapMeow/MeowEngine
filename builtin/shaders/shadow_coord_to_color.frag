#version 450

layout (location = 3) in vec3 inShadowCoord;

layout (location = 0) out vec4 outShadowCoord;
layout (location = 1) out vec4 outShadowDepth;

void main()
{
    outShadowCoord = vec4(inShadowCoord.xy, 1.0, 1.0);
    outShadowDepth = vec4(inShadowCoord.z, inShadowCoord.z, inShadowCoord.z, 1.0);
}