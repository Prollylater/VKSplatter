#version 450

layout(set = 0, binding = 0) uniform SceneData{   
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection; 
	vec4 sunlightColor;
} sceneUBO;

layout(push_constant) uniform UniformBufferObjectB {
    mat4 model;
    mat4 view;
    mat4 proj;
} uboB;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

///////////////////////////////////////::
void main() {
    gl_Position = uboB.proj * uboB.view *  vec4(inPosition, 1.0);
    fragNormal =  inNormal; ;
    fragTexCoord = inTexCoord;
}
