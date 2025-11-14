#version 450
GL_EXT_buffer_reference
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform UniformBufferObjectB {
    mat4 model;
    mat4 view;
    mat4 proj;
} uboB;

//SSBO
//layout(set = 0, binding = 0) buffer VertexData {
//    vec3 positions[];    // Array of vertex positions
//    vec3 normals[];      // Array of vertex normals
//    vec2 texCoords[];    // Array of texture coordinates
//    vec3 colors[];       // Array of vertex colors
//};

struct Vertex {

	vec3 position;
	vec3 normal;
	vec2 uv;
}; 

layout(buffer_reference, std430) readonly buffer  //VertexBuffer { 
	Vertex vertices[];
};


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = uboB.proj * uboB.view * uboB.model * vec4(inPosition, 1.0);
    fragColor = inColor
    fragTexCoord = inTexCoord;
}
