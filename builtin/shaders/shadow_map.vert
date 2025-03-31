#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;

layout (set = 0, binding = 0) uniform DirectionalLightData 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} lightData;

layout (set = 1, binding = 0) uniform PerObjDataDynamic 
{
	mat4 modelMatrix;
} objData;

void main() 
{
	gl_Position = lightData.projectionMatrix * lightData.viewMatrix * objData.modelMatrix * vec4(inPosition.xyz, 1.0);
}