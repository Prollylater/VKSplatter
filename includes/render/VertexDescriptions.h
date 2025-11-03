#pragma once
#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>

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

/////////////////////////////////////////////////::
enum VertexFlags : uint32_t
{
    Vertex_None = 0,
    Vertex_Pos = 1 << 0,
    Vertex_Normal = 1 << 1,
    Vertex_UV = 1 << 2,
    // Unecessary
    Vertex_Color = 1 << 3,
    Vertex_Indices = 1 << 4,

};

// Namespace
// Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)

inline VkVertexInputBindingDescription makeVtxInputBinding(
    uint32_t binding, //"Opaque Index of a Buffer More or less"
    uint32_t stride,  // In binded vertex, after how many data we reach next object
    VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);

// Attribute descriptions: type of the attributes passed to the vertex shader,
// which binding ("buffer") to load them from and at which offset
inline VkVertexInputAttributeDescription makeVtxInputAttr(
    uint32_t location,
    uint32_t binding, // Design the buffer the Attributes is bound to
    VkFormat format,
    uint32_t offset);

struct Mesh;

struct VertexFormat
{
    // Attributes and bindins may not always match in size if we use interleaved format
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    bool mInterleaved = true;
    VertexFlags mVertexFlags = Vertex_None;

    VkPipelineVertexInputStateCreateInfo toCreateInfo() const;
};

// This is bad as well
class VertexFormatRegistry
{
public:
    static void registerFormat(const VertexFlags, const VertexFormat &format);

    static void addFormat(const VertexFlags flags);
    static void addFormat(const Mesh &mesh);
    static bool isFormatIn(const VertexFlags flag);

    static const VertexFormat &getFormat(const VertexFlags);

    static VertexFormat generateVertexFormat(VertexFlags flags);

    static VertexFormat generateInterleavedVertexFormat(VertexFlags flags);

private:
    // Todo:
    // Ultimately making it a map of VertexFlag to Vertex Format is safer
    // This should be take care after reworking the pipeline and render pass parrt
    // Mesh should also auomatically be able to find their format through their size matching the position
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
    // Finally learned that reinterpred cast are cool
    template <typename T>
    void appendToBuffer(size_t bufferIndex, const T &value);
};

VertexBufferData buildInterleavedVertexBuffer(const std::vector<Mesh> &meshes, const VertexFormat &format);
VertexBufferData buildSeparatedVertexBuffers(const Mesh &mesh, const VertexFormat &format);
VertexBufferData buildInterleavedVertexBuffer(const Mesh &mesh, const VertexFormat &format);

inline uint32_t calculateVertexStride(VertexFlags flags)
{
    uint32_t stride = 0;

    if (flags & Vertex_Pos)
        stride += sizeof(glm::vec3);
    if (flags & Vertex_Normal)
        stride += sizeof(glm::vec3);
    if (flags & Vertex_UV)
        stride += sizeof(glm::vec2);
    if (flags & Vertex_Color)
        stride += sizeof(glm::vec3);

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
Attributes (shader inputs)
┌─────────────┐     ┌─────────────┐
│ location 0  │     │ location 1  │
│ pos (vec3)  │     │ color (vec4)│
│ binding=0   │     │ binding=1   │
│ offset=0    │     │ offset=0    │
└─────────────┘     └─────────────┘
       │                   │
       └─────── shader vertex input ────────▶


*/