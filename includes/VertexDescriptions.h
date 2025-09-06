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
    Vertex_Color = 1 << 3,
    Vertex_Indices = 1 << 4,

};
// Namespace
inline VkVertexInputBindingDescription makeVtxInputBinding(
    uint32_t binding,
    uint32_t stride,
    VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX);
inline VkVertexInputAttributeDescription makeVtxInputAttr(
    uint32_t location,
    uint32_t binding,
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

struct Mesh
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> colors;
    std::vector<uint32_t> indices;

    // Awful really
    VertexFlags inputFlag;
    // BufferHandle vertexBuffer
    // BufferHandle indexBuffer;
    // uint32_t indexCount;
    size_t vertexCount() const
    {
        return positions.size(); // assume others match or are empty
    }

    const VertexFormat &getFormat() const
    {
        return VertexFormatRegistry::getFormat(inputFlag);
    }

    const VertexFlags &getflag() const
    {
        return inputFlag;
    }

    bool validateMesh(VertexFlags flags)
    {
        size_t count = positions.size();
        if ((flags & Vertex_Normal) && normals.size() != count)
            return false;
        if ((flags & Vertex_Color) && colors.size() != count)
            return false;
        if ((flags & Vertex_UV) && uvs.size() != count)
            return false;
        return true;
    }

    void loadModel(std::string);
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


{
  "name": "PosNormalUV",
  "binding": {
    "binding": 0,
    "stride": 32,
    "inputRate": "vertex"
  },
  "attributes": [
    {
      "location": 0,
      "binding": 0,
      "format": "R32G32B32_SFLOAT",
      "offset": 0
    },
    {
      "location": 1,
      "binding": 0,
      "format": "R32G32B32_SFLOAT",
      "offset": 12
    },
    {
      "location": 2,
      "binding": 0,
      "format": "R32G32_SFLOAT",
      "offset": 24
    }
  ]
}

---

### 🧠 Loader Code

```cpp
#include <nlohmann/json.hpp>

VkFormat parseFormatString(const std::string& fmt) {
    static std::unordered_map<std::string, VkFormat> formatMap = {
        {"R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT},
        {"R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT},
        // Add more as needed
    };
    return formatMap.at(fmt);
}

VertexFormat loadVertexFormatFromJSON(const nlohmann::json& j) {
    VertexFormat format;

    // Binding
    const auto& b = j["binding"];
    format.binding.binding = b["binding"];
    format.binding.stride = b["stride"];
    format.binding.inputRate = (b["inputRate"] == "instance") ?
        VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

    // Attributes
    for (const auto& attr : j["attributes"]) {
        VkVertexInputAttributeDescription d{};
        d.location = attr["location"];
        d.binding = attr["binding"];
        d.format = parseFormatString(attr["format"]);
        d.offset = attr["offset"];
        format.attributes.push_back(d);
    }

    return format;
}
```

---

```

### 🔧 C++ Parse Example (with `nlohmann/json`)

```cpp
VertexFormat loadFormatFromJSON(const nlohmann::json& j);
```

You then `registerFormat(name, loadedFormat)` to plug it into your system.

---

# ⚙️ 4. Pipeline Cache Design

## ✅ Goals

* Cache pipelines per shader + vertex format
* Efficient reuse; variant support

### 🔧 Design Concept

```cpp
struct PipelineKey {
    std::string shaderID;
    std::string vertexFormat;

    bool operator==(const PipelineKey& other) const {
        return shaderID == other.shaderID && vertexFormat == other.vertexFormat;
    }
};

namespace std {
template <>
struct hash<PipelineKey> {
    std::size_t operator()(const PipelineKey& k) const {
        return hash<string>()(k.shaderID) ^ hash<string>()(k.vertexFormat);
    }
};
}
```

```cpp
class PipelineCache {
public:
    VkPipeline getOrCreatePipeline(const PipelineKey& key);
private:
    std::unordered_map<PipelineKey, VkPipeline> pipelineMap;
};
```

**Bonus**: Add support for pipeline derivatives or libraries for efficiency.

---

Yes — **absolutely**. You're on the right track.

In real engines, the **vertex format isn't just a global thing** — it’s often tightly tied to **assets**, like a `Mesh`, `SubMesh`, or even a `Material`. Here's the big idea:

---






```

This is parsed at load time, and you assign the `vertexFormatName` accordingly.

---


# ✅ PART 2: Shader + Reflection + Format Matcher


# 🧠 2. Shader Reflection-Based Layout Extraction (SPIRV-Cross)

## ✅ Goals

* Use SPIRV-Cross to read input attributes
* Match against registry or auto-generate

### 🔧 Sample Code (using SPIRV-Cross)

```cpp
spirv_cross::Compiler comp(spirvBinary);
auto resources = comp.get_shader_resources();

for (const auto& input : resources.stage_inputs) {
    auto loc = comp.get_decoration(input.id, spv::DecorationLocation);
    auto type = comp.get_type(input.type_id);
    std::string name = input.name;

    // Detect type, vector size, etc.
    // You can use this to auto-fill VkVertexInputAttributeDescription
}
```

You can also **generate a VertexFormat struct** directly from reflection or **verify consistency** against registered formats.

---
#include <spirv_cross/spirv_cross.hpp>

struct ShaderInputAttribute {
    uint32_t location;
    std::string name;
    VkFormat expectedFormat; // Optional
};

std::vector<ShaderInputAttribute> extractInputsFromShader(const std::vector<uint32_t>& spirv) {
    spirv_cross::Compiler comp(spirv);
    auto resources = comp.get_shader_resources();

    std::vector<ShaderInputAttribute> result;

    for (const auto& input : resources.stage_inputs) {
        auto loc = comp.get_decoration(input.id, spv::DecorationLocation);
        auto type = comp.get_type(input.type_id);
        uint32_t vecSize = type.vecsize;

        // Simple match (expand to handle matrix, integer, etc.)
        VkFormat fmt = VK_FORMAT_UNDEFINED;
        if (type.basetype == spirv_cross::SPIRType::Float && vecSize == 3)
            fmt = VK_FORMAT_R32G32B32_SFLOAT;
        else if (vecSize == 2)
            fmt = VK_FORMAT_R32G32_SFLOAT;

        result.push_back({ loc, input.name, fmt });
    }

    return result;
}
```

---

### ✅ Validation Example

```cpp
bool validateFormatAgainstShader(const VertexFormat& fmt, const std::vector<ShaderInputAttribute>& inputs) {
    for (const auto& attr : inputs) {
        auto it = std::find_if(fmt.attributes.begin(), fmt.attributes.end(), [&](const auto& a) {
            return a.location == attr.location && a.format == attr.expectedFormat;
        });
        if (it == fmt.attributes.end()) {
            std::cerr << "Mismatch at location " << attr.location << ": missing or wrong format\n";
            return false;
        }
    }
    return true;
}
```

---
*/
