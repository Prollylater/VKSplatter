#version 450

layout(push_constant) uniform UniformBufferObjectB {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 vertNormals;

void main() {
    gl_Position = ubo.view * ubo.model * vec4(inPosition, 1.0);
    vertNormals = ubo.view * ubo.model * vec4(inNormal, 0.0) *0.2;
}
