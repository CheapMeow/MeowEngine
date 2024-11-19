#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;

layout (set = 0, binding = 0) uniform PerSceneData 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} sceneData;

layout (set = 3, binding = 0) uniform PerObjDataDynamic
{
	mat4 modelMatrix;
} objData;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outPosition;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	mat3 normalMatrix = transpose(inverse(mat3(objData.modelMatrix)));
	vec3 normal = normalize(normalMatrix * inNormal);

	outNormal   = normal;
	outPosition = (objData.modelMatrix * vec4(inPosition.xyz, 1.0)).xyz;

	gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * objData.modelMatrix * vec4(inPosition.xyz, 1.0);
}
