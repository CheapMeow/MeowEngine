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

layout (set = 2, binding = 0) uniform PerObjDataDynamic 
{
	mat4 modelMatrix;
} objData;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV0;
layout (location = 3) out vec4 outShadowCoord;

void main() 
{
	mat3 normalMatrix = transpose(inverse(mat3(objData.modelMatrix)));

	vec4 worldPos = objData.modelMatrix * vec4(inPosition.xyz, 1.0);
	gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * worldPos;

    outPosition = worldPos.xyz;
	outNormal = normalize(normalMatrix * inNormal);
    outUV0 = inUV0;

	outShadowCoord = lightData.projectionMatrix * lightData.viewMatrix * worldPos;
}
