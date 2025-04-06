#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;

layout (set = 0, binding = 0) uniform PerSceneData 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} sceneData;

layout (set = 0, binding = 1) uniform DirectionalLightData 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} lightData;

layout (set = 1, binding = 0) uniform PerObjDataDynamic 
{
	mat4 modelMatrix;
} objData;

layout (location = 0) out vec3 outShadowCoord;

void main() 
{
	mat3 normalMatrix = transpose(inverse(mat3(objData.modelMatrix)));

	vec4 worldPos = objData.modelMatrix * vec4(inPosition.xyz, 1.0);
	gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * worldPos;

	vec4 shadowProj = lightData.projectionMatrix * lightData.viewMatrix * worldPos;
	outShadowCoord.xyz = shadowProj.xyz / shadowProj.w;
	// [-1, 1] -> [0, 1]
	outShadowCoord.xy = outShadowCoord.xy * 0.5 + 0.5;
	// flip y
	outShadowCoord.y = 1.0 - outShadowCoord.y;
}