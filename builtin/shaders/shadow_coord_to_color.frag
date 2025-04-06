#version 450

layout (location = 0) in vec4 inShadowCoord;

layout (location = 0) out vec4 outShadowCoord;
layout (location = 1) out vec4 outShadowDepth;

void main()
{
    vec3 ShadowCoord = inShadowCoord.xyz / inShadowCoord.w;
	// [-1, 1] -> [0, 1]
	ShadowCoord.xy = ShadowCoord.xy * 0.5 + 0.5;
	// flip y
	ShadowCoord.y = 1.0 - ShadowCoord.y;

    outShadowCoord = vec4(ShadowCoord.xy, 1.0, 1.0);
    outShadowDepth = vec4(ShadowCoord.z, ShadowCoord.z, ShadowCoord.z, 1.0);
}