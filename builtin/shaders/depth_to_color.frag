#version 450

layout (location = 0) in vec2 inUV0;

layout (set = 0, binding = 0) uniform sampler2D inputDepth;

layout (location = 0) out vec4 outFragColor;

void main()
{		
    float depth = texture(inputDepth, inUV0).r;
    outFragColor = vec4(depth, depth, depth, 1.0);
}
