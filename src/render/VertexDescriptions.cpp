#include "VertexDescriptions.h"

#include <tiny_obj_loader.h>
#include "Mesh.h"
constexpr uint32_t LOCATION_POS = 0;
constexpr uint32_t LOCATION_NORMAL = 1;
constexpr uint32_t LOCATION_UV = 2;
constexpr uint32_t LOCATION_COLOR = 3;




VkPipelineVertexInputStateCreateInfo VertexFormat::toCreateInfo() const
{
    VkPipelineVertexInputStateCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    info.vertexBindingDescriptionCount = bindings.size();
    info.vertexAttributeDescriptionCount = attributes.size();
    info.pVertexBindingDescriptions = bindings.data();
    info.pVertexAttributeDescriptions = attributes.data();
    return info;
}

void VertexFormatRegistry::registerFormat(const VertexFlags flag, const VertexFormat &format)
{
    getFormats()[flag] = format;
}

bool VertexFormatRegistry::isFormatIn(const VertexFlags flag)
{
    return !(getFormats().find(flag) == getFormats().end());
}
const VertexFormat &VertexFormatRegistry::getFormat(VertexFlags flag)
{
    return getFormats().at(flag);
}


const VertexFormat &VertexFormatRegistry::getStandardFormat()
{
    return getFormats().at(STANDARD_STATIC_FLAG);
}

void VertexFormatRegistry::addFormat(const VertexFlags flag)
{
    if (isFormatIn(flag))
    {
        return;
    }

    // VertexFormat formatS = generateVertexFormat(flags);
    VertexFormat formatPC = generateInterleavedVertexFormat(flag);

    VertexFormatRegistry::registerFormat(flag, formatPC);
}

void VertexFormatRegistry::addFormat(const Mesh &mesh)
{
    addFormat(mesh.inputFlag);
}

std::unordered_map<VertexFlags, VertexFormat> &VertexFormatRegistry::getFormats()
{
    // Lazy initialization
    static std::unordered_map<VertexFlags, VertexFormat> formats;
    return formats;
}

VertexFormat VertexFormatRegistry::generateVertexFormat(VertexFlags flags)
{
    // Even here binding and location must remain coherent else undefined behavior
    // Binding is probably not possible
    VertexFormat vf;
    uint32_t bindingIndex = 0;
    vf.mVertexFlags = flags;
    vf.mInterleaved = false;
    
    if (flags & Vertex_Pos)
    {
        vf.bindings.push_back(makeVtxInputBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_POS, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
    }

    if (flags & Vertex_Normal)
    {
        vf.bindings.push_back(makeVtxInputBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_NORMAL, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
    }

    if (flags & Vertex_UV)
    {
        vf.bindings.push_back(makeVtxInputBinding(bindingIndex, sizeof(glm::vec2)));
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_UV, bindingIndex, VK_FORMAT_R32G32_SFLOAT, 0));
        ++bindingIndex;
    }

    if (flags & Vertex_Color)
    {
        vf.bindings.push_back(makeVtxInputBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_COLOR, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
    }

    // Move ?
    return vf;
}

VertexFormat VertexFormatRegistry::generateInterleavedVertexFormat(VertexFlags flagss)
{
    // Always use full flag instead of worrying about multiples pipelines for now
    VertexFlags flags = STANDARD_STATIC_FLAG;
    VertexFormat vf;
    uint32_t offset = 0;
    uint32_t stride = 0;
    vf.mVertexFlags = flags;
    vf.mInterleaved = true;

    if (flags & Vertex_Pos)
    {
        stride += sizeof(glm::vec3);
    }
    if (flags & Vertex_Normal)
    {
        stride += sizeof(glm::vec3);
    }
    if (flags & Vertex_UV)
    {
        stride += sizeof(glm::vec2);
    }
    if (flags & Vertex_Color)
    {
        stride += sizeof(glm::vec3);
    }

    vf.bindings.push_back(makeVtxInputBinding(0, stride));

    if (flags & Vertex_Pos)
    {
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_POS, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }

    if (flags & Vertex_Normal)
    {
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_NORMAL, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }

    if (flags & Vertex_UV)
    {
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_UV, 0, VK_FORMAT_R32G32_SFLOAT, offset));
        offset += sizeof(glm::vec2);
    }

    if (flags & Vertex_Color)
    {
        vf.attributes.push_back(makeVtxInputAttr(LOCATION_COLOR, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }
    return vf;
}

// Vertex Buffer Manipulation
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

    // Default fallback values
    const glm::vec3 defaultVec3(0.0f);
    const glm::vec2 defaultVec2(0.0f);

    

    for (size_t i = 0; i < vertexCount; ++i)
    {

        // Position
        if (format.mVertexFlags & Vertex_Pos)
        {
            const glm::vec3 &pos = (i < mesh.positions.size()) ? mesh.positions[i] : defaultVec3;
            vbd.appendToBuffer(0, pos);
        }

        // Normal
        if (format.mVertexFlags & Vertex_Normal)
        {
            const glm::vec3 &normal = (i < mesh.normals.size()) ? mesh.normals[i] : defaultVec3;
            vbd.appendToBuffer(0, normal);
        }

        // UV
        if (format.mVertexFlags & Vertex_UV)
        {
            const glm::vec2 &uv = (i < mesh.uvs.size()) ? mesh.uvs[i] : defaultVec2;
            vbd.appendToBuffer(0, uv);
        }

        // Color
        if (format.mVertexFlags & Vertex_Color)
        {
            const glm::vec3 &color = (i < mesh.colors.size()) ? mesh.colors[i] : defaultVec3;
            vbd.appendToBuffer(0, color);
        }
    }

    return vbd;
}