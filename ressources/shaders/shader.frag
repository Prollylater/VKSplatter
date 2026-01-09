#version 450

#define PI 3.14159
uint Has_ColorTexture     = 1 << 0;
uint Has_NormalTexture    = 1 << 1;
uint Has_RoughnessTexture = 1 << 2;
uint Has_Metallic_Texture = 1 << 3;
uint Has_EmissiveTexture =  1 << 4;

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform SceneData{  
	mat4 viewproj;
    vec4 eye;
	vec4 ambientColor;
} sceneUBO;


struct DirectionalLight {
    vec4 direction; 
    vec4 color;     
    float intensity; 
};

layout(std140,set = 0, binding = 1) uniform DirLightsUBO {
    DirectionalLight lights[10]; 
    uint count;
} dirLightsUBO ;


struct PointLight {
    vec4 position;    
    vec4 color;       
    float intensity;  
    float radius;     
};

layout(std430,set = 0,  binding = 2) buffer PointLightsBuffer {
    PointLight lights[10]; //[] is not supported by glsl
    uint count;
} ptLightsBuffer;

layout(set = 1, binding = 0) uniform MaterialUBO{   
	vec4 albedoColor;
	vec4 emissive;
	float metallic;
	float roughness;
	float specular;
	uint flags;
} materialUBO;

layout(set = 1, binding = 1) uniform sampler2D albedoSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D metallicSampler;
layout(set = 1, binding = 4) uniform sampler2D roughnessSampler;
layout(set = 1, binding = 5) uniform sampler2D emissiveSampler;

layout(location = 0) out vec4 outColor;

//Could mutualize most of this but
vec3 computePointLight(vec3 fragPos, vec3 N, PointLight light) {
    vec3 L = light.position.xyz - fragPos ;
    float dist = length(L);
    L = normalize(L);

    vec3 V = normalize(sceneUBO.eye.xyz - fragPos); 
    vec3 H = normalize(L + V);

    float attenuation = 1.0 / (dist * dist);

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * light.color.xyz * light.intensity * attenuation;

    float spec = pow(max(dot(N, H), 0.0), 16.0); 
    vec3 specular = spec * light.color.xyz * light.intensity * attenuation;

    return diffuse + specular;
}

vec3 computeDirectionalLight(vec3 N, vec3 V, DirectionalLight light) {
    vec3 L = normalize(-light.direction.xyz);
    vec3 H = normalize(L + V); 
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * light.color.xyz * light.intensity;

    float spec = pow(max(dot(N, H), 0.0), 16.0);
    vec3 specular = spec * light.color.xyz * light.intensity;
    return diffuse + specular;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(sceneUBO.eye.xyz - fragPos);

    vec4 albedoColor = materialUBO.albedoColor;
    if ( ( materialUBO.flags & Has_ColorTexture ) != 0 ) {
        albedoColor = texture( albedoSampler, fragTexCoord );
    }
    
 vec3 finalColor = vec3(0.0);

    for (int i = 0; i < dirLightsUBO.count; ++i) {
        DirectionalLight dirLight = dirLightsUBO.lights[i];
        finalColor += computeDirectionalLight(N, V, dirLight) * albedoColor.xyz;
    }

       for (int i = 0; i < ptLightsBuffer.count; ++i) {
          PointLight ptLight = ptLightsBuffer.lights[i];
          finalColor += computePointLight(fragPos, N, ptLight)* albedoColor.xyz;
  }
   // outColor = vec4(finalColor, albedoColor.a);

    outColor = vec4(albedoColor);
}
