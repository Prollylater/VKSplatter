#pragma once
#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>


//This is mostly deprecated and you should consider removing it 
/*
[ Px Py Pz Cx Cy Cz | Px Py Pz Cx Cy Cz | ... ]
→ One buffer, one binding  for ?

positions = [ Px Py Pz | Px Py Pz | ... ]
colors    = [ Cx Cy Cz | Cx Cy Cz | ... ]
→ Multiple buffers, one per attribute, multiple bindings
Separate is not implemented in the other part

    float: VK_FORMAT_R32_SFLOAT
    vec2: VK_FORMAT_R32G32_SFLOAT
    vec3: VK_FORMAT_R32G32B32_SFLOAT
    vec4: VK_FORMAT_R32G32B32A32_SFLOAT
*/

//Handle instancing


struct Mesh;


class VertexFormatRegistry
{
public:
    VertexFormatRegistry() = default;
    ~VertexFormatRegistry() = default;

    static void registerFormat(const VertexFlags, const VertexFormat &format);

    static void addFormat(const VertexFlags flags);
    static void addFormat(const Mesh &mesh);
    static bool isFormatIn(const VertexFlags flag);

    static const VertexFormat &getFormat(const VertexFlags);
    static const VertexFormat &getStandardFormat();

    static VertexFormat generateVertexFormat(VertexFlags flags);
    static VertexFormat generateInterleavedVertexFormat(VertexFlags flags);

private:
    static std::unordered_map<VertexFlags, VertexFormat> &getFormats();
};

// Help for loading Vertex
struct VertexUnique
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    bool operator==(const VertexUnique &other) const
    {
        return position == other.position &&
               normal == other.normal &&
               uv == other.uv;
    }
};

struct VertexHash
{
    std::size_t operator()(const VertexUnique &v) const
    {
        std::size_t h1 = std::hash<float>()(v.position.x) ^ (std::hash<float>()(v.position.y) << 1) ^ (std::hash<float>()(v.position.z) << 2);
        std::size_t h2 = std::hash<float>()(v.normal.x) ^ (std::hash<float>()(v.normal.y) << 1) ^ (std::hash<float>()(v.normal.z) << 2);
        std::size_t h3 = std::hash<float>()(v.uv.x) ^ (std::hash<float>()(v.uv.y) << 1);
        return h1 ^ h2 ^ h3;
    }
};

// Vertex Data used for creation
struct VertexBufferData
{
    std::vector<std::vector<uint8_t>> mBuffers;

    template <typename T>
    void appendToBuffer(size_t bufferIndex, const T &value);
};

VertexBufferData buildSeparatedVertexBuffers(const Mesh &mesh, const VertexFormat &format);
VertexBufferData buildInterleavedVertexBuffer(const Mesh &mesh, const VertexFormat &format);

inline uint32_t calculateVertexStride(VertexFlags flags)
{
    uint32_t stride = 0;

    if (flags & Vertex_Pos){
        stride += sizeof(glm::vec3);}
    if (flags & Vertex_Normal){
        stride += sizeof(glm::vec3);}
    if (flags & Vertex_UV){
        stride += sizeof(glm::vec2);}
    if (flags & Vertex_Color){
        stride += sizeof(glm::vec3);}

    return stride;
}

/////////////////////////////////////////////////::
/*
Vertex Buffers (bound via vkCmdBindVertexBuffers)
┌─────────────┐     ┌─────────────┐
│ Binding 0   │     │ Binding 1   │
│  positions  │     │   colors    │
│  stride=12  │     │  stride=16  │
└─────────────┘     └─────────────┘
       │                   │
       │                   │
       ▼                   ▼
Attributes for the shader
┌─────────────┐     ┌─────────────┐
│ location 0  │     │ location 1  │
│ pos (vec3)  │     │ color (vec4)│
│ binding=0   │     │ binding=1   │
│ offset=0    │     │ offset=0    │
└─────────────┘     └─────────────┘
       │                   │
       └─────── shader vertex input ────────▶


*/


//Todo: I should take cue from this to readapt VertexFormat
//Perhaps merge the two classes
enum class InstanceFieldType {
    Float,
    Vec3,
    Vec4,
    Mat4,
    Uint32
};


enum class InstanceFieldUsage : uint8_t {
    CPU_ONLY      = 1 << 0,
    GPU_RENDER    = 1 << 1,
    //GPU_COMPUTE   = 1 << 2,
    CPU_GPU_RENDER  = CPU_ONLY | GPU_RENDER,
    //CPU_GPU_COMPUTE = CPU_ONLY | GPU_COMPUTE,
};

//Todo: Array or mapping for type -> size or for standard Attributes or even for InstanceFieldDesc -> VK
//We should generate descriptor from this 

struct InstanceFieldDesc {
    std::string       name;
    InstanceFieldType type;
    uint32_t          offset;
    uint32_t          size;
    //InstanceFieldUsage  
};


//Notes: Given how unintuitive this class is for genericity sake
//We should add more helper or consider removing it
struct InstanceLayout {
    uint32_t stride;
    std::vector<InstanceFieldDesc> fields;
};

struct InstanceFieldHandle {
    uint32_t offset;
    InstanceFieldType type;
    bool valid = false; 
};


static constexpr std::array<uint32_t, 5> sizeOfInstTypeArr = {
    sizeof(float),          // InstanceFieldType::Float
    sizeof(glm::vec3),      // InstanceFieldType::Vec3
    sizeof(glm::vec4),      // InstanceFieldType::Vec4
    sizeof(glm::mat4),      // InstanceFieldType::Mat4
    sizeof(uint32_t)        // InstanceFieldType::Uint32
};

inline uint32_t sizeOfInstType(InstanceFieldType type)
{
    return sizeOfInstTypeArr[static_cast<int>(type)];
}

static constexpr std::array<VkFormat, 5> formatsOfInstTypeArr = {
    VK_FORMAT_R32_SFLOAT,           // InstanceFieldType::Float
    VK_FORMAT_R32G32B32_SFLOAT,     // InstanceFieldType::Vec3
    VK_FORMAT_R32G32B32A32_SFLOAT,  // InstanceFieldType::Vec4
    VK_FORMAT_R32G32B32A32_SFLOAT,  // InstanceFieldType::Mat4
    VK_FORMAT_R32_UINT              // InstanceFieldType::Uint32
};


inline VkFormat formatsOfInstType(InstanceFieldType type)
{
    if (type < InstanceFieldType::Float || type > InstanceFieldType::Uint32) {
        return VK_FORMAT_UNDEFINED;
    }
    return formatsOfInstTypeArr[static_cast<int>(type)];
}


