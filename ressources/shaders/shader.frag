#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec2 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() { 
     // outColor = vec4(fragColor, 1.0);
     outColor = texture(texSampler, fragTexCoord);
     outColor = fragTexCoord * max( 0.0, dot( fragNormal, vec3( 0.58, 0.58, 0.58 ) ) ) + 0.1;
}

