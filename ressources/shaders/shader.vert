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


struct InstanceData {
     vec4 pos_scale;
     vec4 rotation; 
};


layout(set = 2, binding = 0) buffer instancesBuffer{
 InstanceData data[];
} instancesData;


layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

mat4 quatToMat4(vec4 quat)
{
    vec4 nq = normalize(quat);

    float x = nq.x;
    float y = nq.y;
    float z = nq.z;
    float w = nq.w;

    return mat4(
        1.0 - 2.0*y*y - 2.0*z*z,  2.0*x*y + 2.0*w*z,      2.0*x*z - 2.0*w*y,      0.0,
        2.0*x*y - 2.0*w*z,        1.0 - 2.0*x*x - 2.0*z*z, 2.0*y*z + 2.0*w*x,      0.0,
        2.0*x*z + 2.0*w*y,        2.0*y*z - 2.0*w*x,      1.0 - 2.0*x*x - 2.0*y*y, 0.0,
        0.0,                      0.0,                    0.0,                    1.0
    );
}

mat4 translationMat(vec3 pos)
{
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        pos.x, pos.y, pos.z, 1.0
    );
}

mat4 scaleMat(float s)
{
    return mat4(
        s, 0.0, 0.0, 0.0,
        0.0, s, 0.0, 0.0,
        0.0, 0.0, s, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

mat4 worldMatrix(vec3 pos, float scale, vec4 quat)
{
    mat4 T = translationMat(pos);
    mat4 R = quatToMat4(quat);
    mat4 S = scaleMat(scale);

    return T * R * S;
}

///////////////////////////////////////
void main() {// worldMat*
    mat4 worldMat = worldMatrix(instancesData.data[gl_InstanceIndex].pos_scale.xyz, instancesData.data[gl_InstanceIndex].pos_scale.w, instancesData.data[gl_InstanceIndex].rotation);
    gl_Position =(uboB.viewproj * vec4(inPosition + instancesData.data[gl_InstanceIndex].pos_scale.xyz, 1.0));

    fragPos = inPosition;
    //fragNormal = worldMat * vec4(inNormal, 1.0);
    fragNormal =  inNormal;
    fragTexCoord = inTexCoord;

}
