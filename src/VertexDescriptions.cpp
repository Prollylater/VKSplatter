#include "VertexDescriptions.h"

#include <tiny_obj_loader.h>

constexpr uint32_t LOCATION_POS = 0;
constexpr uint32_t LOCATION_NORMAL = 1;
constexpr uint32_t LOCATION_UV = 2;
constexpr uint32_t LOCATION_COLOR = 3;

// There was some problem on the splitted version not that well implemented
/////////////////////////////////////////////////::

void Mesh::loadModel(std::string filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    if (!warn.empty())
    {
        std::cout << "WARN: " << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cerr << "ERR: " << err << std::endl;
    }

    positions.clear();
    normals.clear();
    uvs.clear();
    colors.clear();
    indices.clear();

    bool hasNormals = !attrib.normals.empty();
    bool hasTexcoords = !attrib.texcoords.empty();

    std::unordered_map<VertexUnique, uint32_t, VertexHash> uniqueVertices;

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            glm::vec3 pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};

            glm::vec3 norm = glm::vec3(0.0f);
            if (hasNormals && index.normal_index >= 0)
            {
                norm = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]};
            }

            glm::vec2 uv = glm::vec2(0.0f);
            if (hasTexcoords && index.texcoord_index >= 0)
            {
                uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
            }

            VertexUnique vertex = {pos, norm, uv};

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(positions.size());
                positions.push_back(pos);
                normals.push_back(norm);
                uvs.push_back(uv);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    std::cout << "Model loaded successfully: " << filename << std::endl;
    std::cout << " - Total unique vertices: " << positions.size() << std::endl;
    std::cout << " - Total indices: " << indices.size() << std::endl;
    std::cout << " - Total shapes: " << shapes.size() << std::endl;
    std::cout << " - Total materials: " << materials.size() << std::endl;

    std::cout << " - Vertex attributes loaded: ";

    // Set flags based on what's filled
    inputFlag = Vertex_None;
    if (!positions.empty())
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Pos);
    if (!normals.empty())
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Normal);
    if (!uvs.empty())
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_UV);
    if (!colors.empty())
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Color);
    if (!indices.empty())
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Indices);
}

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
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeAttr(LOCATION_POS, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
    }

    if (flags & Vertex_Normal)
    {
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeAttr(LOCATION_NORMAL, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
    }

    if (flags & Vertex_UV)
    {
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec2)));
        vf.attributes.push_back(makeAttr(LOCATION_UV, bindingIndex, VK_FORMAT_R32G32_SFLOAT, 0));
        ++bindingIndex;
    }

    if (flags & Vertex_Color)
    {
        vf.bindings.push_back(makeBinding(bindingIndex, sizeof(glm::vec3)));
        vf.attributes.push_back(makeAttr(LOCATION_COLOR, bindingIndex, VK_FORMAT_R32G32B32_SFLOAT, 0));
        ++bindingIndex;
    }

    // Move ?
    return vf;
}

VertexFormat VertexFormatRegistry::generateInterleavedVertexFormat(VertexFlags flagss)
{
    // Always use full flag instead of worrying about multiples pipelines for now
    VertexFlags flags = static_cast<VertexFlags>(Vertex_Pos | Vertex_Normal | Vertex_UV | Vertex_Color);

    VertexFormat vf;
    uint32_t offset = 0;
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
        vf.attributes.push_back(makeAttr(LOCATION_POS, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }

    if (flags & Vertex_Normal)
    {
        vf.attributes.push_back(makeAttr(LOCATION_NORMAL, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }

    if (flags & Vertex_UV)
    {
        vf.attributes.push_back(makeAttr(LOCATION_UV, 0, VK_FORMAT_R32G32_SFLOAT, offset));
        offset += sizeof(glm::vec2);
    }

    if (flags & Vertex_Color)
    {
        vf.attributes.push_back(makeAttr(LOCATION_COLOR, 0, VK_FORMAT_R32G32B32_SFLOAT, offset));
        offset += sizeof(glm::vec3);
    }

    return vf;
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

VertexBufferData buildInterleavedVertexBuffer(const std::vector<Mesh> &meshes, const VertexFormat &format)
{
    VertexBufferData vbd;

    // Calculate total vertex count across all meshes
    size_t totalVertexCount = 0;
    for (const auto &mesh : meshes)
    {
        totalVertexCount += mesh.vertexCount();
    }
    size_t stride = format.bindings[0].stride;

    vbd.mBuffers.resize(format.bindings.size());
    vbd.mBuffers[0].resize(totalVertexCount * stride);
    uint8_t *buffer = vbd.mBuffers[0].data();

    const glm::vec3 defaultVec3(0.0f);
    const glm::vec2 defaultVec2(0.0f);

    // Keep track of the running vertex index across all meshes
    size_t vertexIndex = 0;

    for (const auto &mesh : meshes)
    {
        size_t meshVertexCount = mesh.vertexCount();

        for (size_t i = 0; i < meshVertexCount; ++i)
        {
            if (format.mVertexFlags & Vertex_Pos)
            {
                const glm::vec3 &pos = (i < mesh.positions.size()) ? mesh.positions[i] : defaultVec3;
                vbd.appendToBuffer(0, pos);
            }

            if (format.mVertexFlags & Vertex_Normal)
            {
                const glm::vec3 &normal = (i < mesh.normals.size()) ? mesh.normals[i] : defaultVec3;
                vbd.appendToBuffer(0, normal);
            }

            if (format.mVertexFlags & Vertex_UV)
            {
                const glm::vec2 &uv = (i < mesh.uvs.size()) ? mesh.uvs[i] : defaultVec2;
                vbd.appendToBuffer(0, uv);
            }

            if (format.mVertexFlags & Vertex_Color)
            {
                const glm::vec3 &color = (i < mesh.colors.size()) ? mesh.colors[i] : defaultVec3;
                vbd.appendToBuffer(0, color);
            }
        }
    }

    return vbd;
}
