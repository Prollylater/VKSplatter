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

//Todo: Alignemnt and quantize lights structure ?
struct DirectionalLight {
    vec4 direction; 
    vec4 color;     
    float intensity; 
};

layout(std430,set = 0, binding = 1) buffer DirLightsUBO {
    DirectionalLight lights[5]; 
    uint count;
} dirLightsUBO ;


struct PointLight {
    vec4 position;    
    vec4 color;       
    float intensity;  
    float radius;     
};

layout(std430,set = 0,  binding = 2) buffer PointLightsBuffer { 
    PointLight lights[5];
    uint count;
} ptLightsBuffer;


layout(set = 0, binding = 3) uniform sampler2DArray cShadowMaps; 

struct Cascade {
    mat4 cascadeVP;
    float cascadeSplits;   
};

layout(std430, set = 0, binding = 4) buffer CascadedShadowUBO {
    Cascade cascades[3];
} cascadeSSBO;


layout(set = 2, binding = 0) uniform MaterialUBO{   
	vec4 albedoColor;
	vec4 emissive;
	float metallic;
	float roughness;
	float specular;
	uint flags;
} materialUBO;

layout(set = 2, binding = 1) uniform sampler2D albedoSampler;
layout(set = 2, binding = 2) uniform sampler2D normalSampler;
layout(set = 2, binding = 3) uniform sampler2D metallicSampler;
layout(set = 2, binding = 4) uniform sampler2D roughnessSampler;
layout(set = 2, binding = 5) uniform sampler2D emissiveSampler;

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

	float spec = pow(max(dot(N, H), 0.0), 16.0) * materialUBO.specular;
    vec3 specular = spec * light.color.xyz * light.intensity * attenuation;

    return diffuse + specular;
}

vec3 computeDirectionalLight(vec3 N, vec3 V, DirectionalLight light) {
    vec3 L = normalize(-light.direction.xyz);
    vec3 H = normalize(L + V); 
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * light.color.xyz * light.intensity;

	float spec = pow(max(dot(N, H), 0.0), 16.0) * materialUBO.specular;
    vec3 specular = spec * light.color.xyz * light.intensity;
    return diffuse + specular;
}

float computeCascadeShadow(vec3 fragPosWorld, vec3 normal, vec3 lightDir)
{

    vec4 fragPosView = sceneUBO.viewproj * vec4(fragPosWorld, 1.0);
    float depth = abs(fragPosView.z);


    int cascadeIndex = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (depth < cascadeSSBO.cascades[i].cascadeSplits)
        {
            cascadeIndex = i;
            break;
        }
    }

    Cascade cascade = cascadeSSBO.cascades[cascadeIndex];


    vec4 fragPosLightSpace = cascade.cascadeVP * vec4(fragPosWorld, 1.0);

    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // Outside shadow map
    if (projCoords.z > 1.0)
        return 1.0;

    // Bias (important to avoid shadow acne)
    float bias = max(0.0005 * (1.0 - dot(normal, -lightDir)), 0.00005);
    return bias;
    float shadowDepth = texture(
        cShadowMaps,
        vec3(projCoords.xy, cascadeIndex)
    ).r;

    float currentDepth = projCoords.z;


    float shadow = currentDepth - bias > shadowDepth ? 0.0 : 1.0;

    return shadow;
}

void main() {
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(sceneUBO.eye.xyz - fragPos);

    vec4 albedoColor = materialUBO.albedoColor;
    if ( ( materialUBO.flags & Has_ColorTexture ) != 0 ) {
        albedoColor = texture( albedoSampler, fragTexCoord );
    }
    
vec3 finalColor = sceneUBO.ambientColor.rgb * albedoColor.rgb;

    for (int i = 0; i < dirLightsUBO.count; ++i)
	{
	    DirectionalLight dirLight = dirLightsUBO.lights[i];

	    vec3 L = normalize(-dirLight.direction.xyz);

	    float shadow = computeCascadeShadow(fragPos, N, L);

	    finalColor += computeDirectionalLight(N, V, dirLight)
		          * albedoColor.xyz;
		          //* shadow;
	}
       for (int i = 0; i < ptLightsBuffer.count; ++i) {
          PointLight ptLight = ptLightsBuffer.lights[i];
          finalColor += computePointLight(fragPos, N, ptLight)* albedoColor.xyz;
  }
   outColor = vec4(finalColor, albedoColor.a);
}
