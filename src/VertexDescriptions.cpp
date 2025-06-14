#include "VertexDescriptions.h"


//There was some problem on the splitted version not that well implemented
/////////////////////////////////////////////////::
VkVertexInputBindingDescription makeBinding(
    uint32_t binding,
    uint32_t stride,
    VkVertexInputRate inputRate)
{
    VkVertexInputBindingDescription desc{};
    desc.binding = binding;
    desc.stride = stride;
    desc.inputRate = inputRate;
    return desc;
}

VkVertexInputAttributeDescription makeAttr(
    uint32_t location,
    uint32_t binding,
    VkFormat format,
    uint32_t offset)
{
    VkVertexInputAttributeDescription desc{};
    desc.location = location;
    desc.binding = binding;
    desc.format = format;
    desc.offset = offset;
    return desc;
}

VkPipelineVertexInputStateCreateInfo VertexFormat::toCreateInfo() const
{
    VkPipelineVertexInputStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount = bindings.size();
    info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    info.pVertexBindingDescriptions = bindings.data();
    info.pVertexAttributeDescriptions = attributes.data();
    return info;
}

void VertexFormatRegistry::registerFormat(const std::string &name, const VertexFormat &format)
{
    getFormats()[name] = format;
}

const VertexFormat &VertexFormatRegistry::getFormat(const std::string &name)
{
    return getFormats().at(name);
}

VertexFormat VertexFormatRegistry::generateVertexFormat(VertexFlags flags)
{
    // Even here binding and location must remain coherent else undefined behavior
    // Binding is probably not possible
    VertexFormat vf;
    uint32_t bindingIndex = 0;
    uint32_t location = 0;
    vf.mVertexFlags = flags;
    vf.mInterleaved = false;

    if (flags & Vertex_Pos)
    {
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeAttr(location, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
        ++location;
    }

    if (flags & Vertex_Normal)
    {
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeAttr(location, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
        ++location;
    }

    if (flags & Vertex_UV)
    {
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec2)));
        vf.attributes.push_back(makeAttr(location, bindingIndex, VK_FORMAT_R32G32_SFLOAT, 0));
        ++bindingIndex;
        ++location;
    }

    if (flags & Vertex_Color)
    {
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeAttr(location, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
        ++location;
    }

    // Move ?
    return vf;
}

VertexFormat VertexFormatRegistry::generateInterleavedVertexFormat(VertexFlags flags)
{
    VertexFormat vf;
    uint32_t offset = 0;
    uint32_t location = 0;
    uint32_t stride = 0;
    vf.mVertexFlags = flags;
    vf.mInterleaved = true;

    if (flags & Vertex_Pos)
        stride += sizeof(glm::vec3);
    if (flags & Vertex_Normal)
        stride += sizeof(glm::vec3);
    if (flags & Vertex_UV)
        stride += sizeof(glm::vec2);
    if (flags & Vertex_Color)
        stride += sizeof(glm::vec3);
   

    vf.bindings.push_back(makeBinding(0, stride));

    if (flags & Vertex_Pos)
    {
        vf.attributes.push_back(makeAttr(location++, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }
    if (flags & Vertex_Normal)
    {
        vf.attributes.push_back(makeAttr(location++, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }

    if (flags & Vertex_UV)
    {
        vf.attributes.push_back(makeAttr(location++, 0, VK_FORMAT_R32G32_SFLOAT, offset));
        offset += sizeof(glm::vec2);
    }
    
    if (flags & Vertex_Color)
    {
        vf.attributes.push_back(makeAttr(location++, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }

    return vf;
}

//Use flag instead of brittle string technically the flag already maatter more thant the string anyway
//Store flag into vertex desctipions ?
void VertexFormatRegistry::initBase()
{
    VertexFlags flagsPC = static_cast<VertexFlags>(Vertex_Pos | Vertex_Color | Vertex_UV);

    // VertexFormat formatS = generateVertexFormat(flags);
    VertexFormat formatPC = generateInterleavedVertexFormat(flagsPC);

    // VertexFormatRegistry::registerFormat("pos_color_split", formatS);
    VertexFormatRegistry::registerFormat("pos_color_interleaved", formatPC);
}

std::unordered_map<std::string, VertexFormat> &VertexFormatRegistry::getFormats()
{
    // Lazy initialization
    static std::unordered_map<std::string, VertexFormat> formats;
    return formats;
}

// Vertex Buffer Manipulation
// Finally learned that reinterpred cast are cool
template <typename T>
void VertexBufferData::appendToBuffer(size_t bufferIndex, const T &value)
{
    const uint8_t *raw = reinterpret_cast<const uint8_t *>(&value);
    // Copy the bytes
    mBuffers[bufferIndex].insert(mBuffers[bufferIndex].end(), raw, raw + sizeof(T));
}

VertexBufferData buildSeparatedVertexBuffers(const Mesh &mesh, const VertexFormat &format)
{
    VertexBufferData vbd;
    vbd.mBuffers.resize(format.bindings.size());

    uint32_t attrIndex = 0;
    // Order is very rigid move it to a loop that switch between the four at some point
    if (format.mVertexFlags & Vertex_Pos)
    {
        for (const auto &pos : mesh.positions)
        {
            vbd.appendToBuffer(attrIndex, pos);
        }
        ++attrIndex;
    }
    if (format.mVertexFlags & Vertex_Normal)
    {
        for (const auto &norm : mesh.normals)
        {
            vbd.appendToBuffer(attrIndex, norm);
        }
        ++attrIndex;
    }
    if (format.mVertexFlags & Vertex_UV)
    {
        for (const auto &uv : mesh.uvs)
        {
            vbd.appendToBuffer(attrIndex, uv);
        }
        ++attrIndex;
    }
    if (format.mVertexFlags & Vertex_Color)
    {
        for (const auto &col : mesh.colors)
        {
            vbd.appendToBuffer(attrIndex, col);
        }
        ++attrIndex;
    }
    return vbd;
}

VertexBufferData buildInterleavedVertexBuffer(const Mesh &mesh, const VertexFormat &format)
{
    VertexBufferData vbd;
    size_t vertexCount = mesh.vertexCount();
    // Size of one vertex should be stored there
    // Also this assume we can't have a separate And an interleavec
    // Todo
    size_t stride = format.bindings[0].stride;
    vbd.mBuffers.resize(format.bindings.size());
    vbd.mBuffers[0].reserve(vertexCount * stride);

    for (size_t i = 0; i < vertexCount; ++i)
    {
        if (format.mVertexFlags & Vertex_Pos)
        {
            vbd.appendToBuffer(0, mesh.positions[i]);
        }
        if (format.mVertexFlags & Vertex_Normal)
        {
            vbd.appendToBuffer(0, mesh.normals[i]);
        }
        if (format.mVertexFlags & Vertex_UV)
        {
            vbd.appendToBuffer(0, mesh.uvs[i]);
        }
        if (format.mVertexFlags & Vertex_Color)
        {
            vbd.appendToBuffer(0, mesh.colors[i]);
        }
    }

    return vbd;
}
