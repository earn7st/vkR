#version 460

layout(location = 0) in vec3 vWorldDir;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform PerFrameUBO {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
} perFrame;

layout(set = 1, binding = 10) uniform samplerCube texCube;

void main()
{
    vec3 color = texture(texCube, normalize(vWorldDir)).rgb;
    outColor = vec4(color, 1.0);
}
