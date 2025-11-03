#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform MaterialUBO{   
	vec4 albedoColor;
	vec4 emissive;
	float metallic;
	float roughness;
} materialUBO;
layout(set = 1, binding = 1) uniform sampler2D albedoSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampelr;
layout(set = 1, binding = 3) uniform sampler2D metallicSampler;
layout(set = 1, binding = 4) uniform sampler2D roughnessSampler;

void main() { 
     // outColor = vec4(fragColor, 1.0);
     outColor = texture(albedoSampler, fragTexCoord);
}

