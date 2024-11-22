#version 450

struct OutPointLight{
	vec3 pos;
	vec3 viewPos;
};

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec3 inTangent;

layout (set = 0, binding = 0) uniform PerSceneData 
{
	mat4 viewMatrix;
	mat4 projectionMatrix;
} sceneData;

layout (set = 1, binding = 0) uniform PointLight{
	vec3 pos;
	vec3 viewPos;
} light;

layout (set = 2, binding = 0) uniform PerObjDataDynamic 
{
	mat4 modelMatrix;
} objData;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec2 outUV0;
layout (location = 2) flat out OutPointLight outLight;

void main() 
{
	mat3 normalMatrix = transpose(inverse(mat3(objData.modelMatrix)));
	vec3 T = normalize(normalMatrix * inTangent);
	vec3 N = normalize(normalMatrix * inNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

	gl_Position = sceneData.projectionMatrix * sceneData.viewMatrix * objData.modelMatrix * vec4(inPosition.xyz, 1.0);
	
	mat3 TBN = transpose(mat3(T, B, N)); 
    outPosition = (objData.modelMatrix * vec4(inPosition.xyz, 1.0)).xyz;
    outUV0 = inUV0;

	outLight.pos = TBN * light.pos;
	outLight.viewPos = TBN * light.viewPos;
}
