#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inUV0;

layout (binding = 0) uniform UBO 
{
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
} uboMVP;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	mat3 normalMatrix = transpose(inverse(mat3(uboMVP.modelMatrix)));
	vec3 normal = normalize(normalMatrix * inNormal.xyz);
	outNormal   = normal;
	outColor = inColor;
	outUV = inUV0;
	gl_Position = uboMVP.projectionMatrix * uboMVP.viewMatrix * uboMVP.modelMatrix * vec4(inPosition.xyz, 1.0);
}
