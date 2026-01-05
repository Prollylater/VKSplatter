#version 450

#define PI 3.14159
uint Has_ColorTexture     = 1 << 0;
uint Has_NormalTexture    = 1 << 1;
uint Has_RoughnessTexture = 1 << 2;
uint Has_Metallic_Texture = 1 << 3;
uint Has_EmissiveTexture =  1 << 4;

//Compare with Frostbite fresnelSchlick
vec3 fSchlick(float cosTheta, float F90, vec3 F0)
{
    return F0 + (F90 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float heaviside(float x) {
    return x < 0.0 ? 0.0 : 1.0;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}



float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, float NdotV, vec3 L, float roughness)
{
    float NdotL = max(dot(N, L), 0.0);
    float ggxV  = GeometrySchlickGGX(NdotV, roughness);
    float ggxL  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggxL * ggxV;
}




layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform SceneData{  
	mat4 viewproj;
    vec4 eye;
	vec4 ambientColor;
	vec4 lightDirection; 
	vec4 lightColor;
} sceneUBO;

layout(set = 1, binding = 0) uniform MaterialUBO{   
	vec4 albedoColor;
	vec4 emissive;
	float metallic;
	float roughness;
	uint flags;
	float padding;
} materialUBO;

//Metallic/Roughness and  AO could share the same Texture
layout(set = 1, binding = 1) uniform sampler2D albedoSampler;
layout(set = 1, binding = 2) uniform sampler2D normalSampler;
layout(set = 1, binding = 3) uniform sampler2D metallicSampler;
layout(set = 1, binding = 4) uniform sampler2D roughnessSampler;
layout(set = 1, binding = 5) uniform sampler2D emissiveSampler;

layout(location = 0) out vec4 outColor;

void main() { 

vec3 N = normalize( fragNormal );
vec3 V = normalize( sceneUBO.eye.xyz - fragPos);

//Light Contribution
//vec3 L = normalize( sceneUBO.light.xyz - fragPos );
vec3 L = normalize(sceneUBO.lightDirection.xyz);
vec3 H = normalize( L + V );


vec4 albedoColor = materialUBO.albedoColor;
    if ( ( materialUBO.flags & Has_ColorTexture ) != 0 ) {
        albedoColor = texture( albedoSampler, fragTexCoord );
    }


if ( ( materialUBO.flags & Has_NormalTexture ) != 0 ) {
        N = normalize( texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0 );
}

float roughness = materialUBO.roughness;
float metalness = materialUBO.metallic;

 if ( ( materialUBO.flags & Has_Metallic_Texture ) != 0 ) {
        vec4 mTex = texture(metallicSampler, fragTexCoord);
        metalness *= mTex.r;

    }


 if ( ( materialUBO.flags & Has_RoughnessTexture ) != 0 ) {
        vec4 rTex = texture(roughnessSampler, fragTexCoord);
        roughness *= rTex.r;

    }



  vec3 emissive = vec3( 0 );
    if ( ( materialUBO.flags & Has_EmissiveTexture ) != 0 ) {
         emissive = texture(emissiveSampler, fragTexCoord).rgb;
    }



//Why this again ?
    float NdotL = clamp( dot(N, L), 0, 1 );
    if ( NdotL > 1e-5 ) {
        float NdotV = dot(N, V);
        float HdotL = dot(H, L);
        float HdotV = dot(H, V); 

        //Microfacet distribution
        //Estimates the area of microfacets aligned to give perfect specular
		float D = DistributionGGX(N, H, roughness);

        //Shadow occlusion term
		float G = GeometrySmith(N, NdotV, L, roughness);   //Microfacet/Geometry trapping the light

        
        //Fresnel Reflection 
        //base reflectivity at normal incidence
        vec3 f0 = vec3(0.04); //See the approximation anew
		f0 = mix(f0, albedoColor.xyz, metalness); 
        
        //base reflectivity at grazing angle
        float f90 =  1.0;// Often 1.0
        //f90 = mix(f90, albedoColor.xyz, metalness); 
        vec3 F = fSchlick(max(HdotV,0.0), f90, f0); //kS

        // Basic Lambertian diffuse for now
        vec3 diffuse = /*(vec3(1.0) - F) **/ (1.0 - metalness) * albedoColor.rgb / PI;
        //vec3 radiance = lightColor.rgb * NdotL  *attenuation;  

        // Specular BRDF
        vec3 numerator    = D * G * F;
        float denominator = 4.0 * max(NdotV, 0.0) * NdotL + 0.0001;
        vec3 specular  = numerator / denominator;  
  
        //finalColor = emissive + mix( material_colour, material_colour * ao, occlusion_factor);
        vec3 finalColor = emissive + ( specular + diffuse);

        outColor = vec4( finalColor, albedoColor.a );

        //BTDF
    } else {
        outColor = albedoColor;
    }
}
