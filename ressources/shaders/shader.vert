#version 450

layout(set = 0, binding = 0) uniform SceneData{  
	mat4 viewproj;
    vec4 eye;
	vec4 ambientColor;
} sceneUBO;

layout(push_constant) uniform UniformBufferObjectB {
    mat4 viewproj;
} uboB;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

///////////////////////////////////////
void main() {
    gl_Position = uboB.viewproj * vec4(inPosition, 1.0);
    
    fragPos = inPosition;
    fragNormal =  inNormal; ;
    fragTexCoord = inTexCoord;
}
