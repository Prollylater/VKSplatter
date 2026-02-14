#include "Scene.h"
#include <unordered_set>

// Todo:: Add user function to handle this 
Scene::Scene()
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneLayout.addDescriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));

    uint32_t lastLight = lights.addDirLight({glm::vec4(0.5, 1.0, 0.0, 0.0), glm::vec4(1.0, 1.0, 1.0, 0.0), 1.0});
    lights.enableShadow(lastLight);
    // lights.addDirLight({glm::vec4(0.5, -1.0, 0.0, 0.0), glm::vec4(0.2, 0.0, 1.0, 0.0), 1.0});
    // lights.addPointLight({glm::vec4(0.0, 1.0, 0.5, 0.0), glm::vec4(1.0, 1.0, 0.0, 0.0), 1.5, 1.5});
    // lights.addPointLight({glm::vec4(-0.0, 0.0, 0.5, 0.0), glm::vec4(0.0, 1.0, 1.0, 0.0), 0.5, 0.5});
    // lights.addPointLight({glm::vec4(-9.5, -14, 0.5, 0.0), glm::vec4(1.0, 1.0, 0.0, 0.0), 2.5, 5.5});
    // lights.addPointLight({glm::vec4(-5.5, -13.0, 0.5, -0.5), glm::vec4(0.0, 1.0, 1.0, 0.0), 5.5, 1.5});
};

Scene::Scene(int compute)
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));
};

void Scene::addNode(SceneNode node)
{
    /*
    InstanceFieldHandle transformField = node.getField("transform");
    if (transformField.valid == true)
    {
        for (uint32_t i = 0; i < node.instanceCount; i++)
        {
            Extents worldExtents = node.nodeExtents;

            glm::mat4 *transformPtr = node.getFieldPtr<glm::mat4>(i, "transform");
            glm::mat4 worldMatrix = *transformPtr;

            worldExtents.translate(worldMatrix[3]);

            sceneBB.expand(worldExtents);
        }
    }
    */

    for (uint32_t i = 0; i < node.instanceCount; i++)
    {
        Extents worldExtents = node.nodeExtents;
        glm::mat4 worldMatrix = node.getTransform(i).getWorldMatrix();
        worldExtents.translate(worldMatrix[3]);
        sceneBB.expand(worldExtents);
    }
    nodes.push_back(node);
}

const SceneNode &Scene::getNode(uint32_t index)
{
    return nodes[index];
};

void Scene::clearScene()
{
    nodes.clear();
}

// Temporary
Camera &Scene::getCamera() { return camera; };
SceneData Scene::getSceneData() const
{
    glm::mat4 proj = camera.getProjectionMatrix();
    proj[1][1] *= -1;

    return {proj * camera.getViewMatrix(), camera.getEye(), glm::vec4(0.3, 0.1, 0.4, 0.0)};
};

LightPacket Scene::getLightPacket() const
{

    const auto &dirInstances = lights.getDirLights();

    std::vector<DirectionalLight> directionalLights;
    directionalLights.reserve(dirInstances.size());

    for (const auto &inst : dirInstances)
    {
        directionalLights.push_back(inst.light);
    };

    const auto &ptInstances = lights.getPointLights();

    std::vector<PointLight> ptLights;
    ptLights.reserve(ptInstances.size());

    for (const auto &inst : ptInstances)
    {
        ptLights.push_back(inst.light);
    };

    return {directionalLights, lights.dirLightCount(), ptLights, lights.pointLightCount()};
}

ShadowPacket Scene::getShadowPacket() const
{

    const auto &dirLights = lights.getDirLights();

    std::vector<std::array<Cascade, MAX_SHDW_CASCADES>> cascades;
    for (const auto &inst : dirLights)
    {
        if (!inst.shadow.has_value())
        {
            continue;
        }

        cascades.push_back(inst.shadow->cascades);
    }

    return {cascades, cascades.size()};
}

void Scene::updateLights(float deltaTime)
{
    lights.update(deltaTime);
    updateShadows();
}

void Scene::updateShadows()
{
    const Camera &cam = camera;
    updateCascadeShadows(lights, cam);
}

// Should only use on a valid scene
void fitCameraToBoundingBox(Camera &camera, const Extents &box)
{
    glm::vec3 center = box.center();
    float radius = box.radius();

    float distance = radius / tan(camera.getFov() * 0.5f);
    distance *= 1.1f;

    glm::vec3 viewDir = glm::normalize(camera.getFront());

    glm::vec3 newPosition = center - viewDir * distance;

    camera.setPosition(newPosition);
}

// Allocator should be tied to SCene or be some sort of Ecs for handling removed stuff
VisibilityFrame extractRenderFrame(const Scene &scene,
                                   const AssetRegistry &registry)
{
    VisibilityFrame frame;
    frame.sceneData = scene.getSceneData();
    frame.lightData = scene.getLightPacket();

    // Assuming Scene new comer were resolved earlier
    //  Notes: We woudl flatten SceneNodes into RenderObjectFrames if we had hierarchy
    for (auto &node : scene.nodes)
    {
        if (!node.visible || node.instanceNb() == 0)
        {
            // We might still want to upload first timer actually
            // In hope to have more contiguous mesh
            continue;
        }

        std::vector<VisibleObject::VisibleInstance> visibleInstances;
        for (uint32_t instIdx = 0; instIdx < node.instanceNb(); ++instIdx)
        {
            glm::vec3 worldPos = node.getTransform(instIdx).getPosition();
            Extents worldExtents = node.nodeExtents;
            worldExtents.translate(worldPos);

            if (!testAgainstFrustum(frame.sceneData.viewproj, worldExtents))
            {
                continue;
            }

            uint64_t key = (uint64_t(&node) << 32) | instIdx;

            VisibleObject::VisibleInstance data{};
            data.transform = node.buildGPUTransform(instIdx);
            data.instanceKey = key;
            // Todo: Always dirty until i handle scene update
            data.transformDirty = true;
            visibleInstances.push_back(data);
        }

        const auto &mesh = registry.get(node.mesh);

        // Create one RenderObjectFrame per submesh, sharing visibleInstances
        // Notes: Ideally visibile instance array wouldn'( be duplicated)
        // Pointer with Linear Allocator
        VisibleObject rof{};
        rof.mesh = node.mesh;
        // rof.indexOffset = 0;
        // rof.vertexOffset = 0;
        rof.visibleInstances = visibleInstances;
        frame.objects.push_back(std::move(rof));
        // rof.instanceData = node.getGenericData(instIdx);
        //  rof.worldExtents = node.nodeExtents;
        // rof.layout = node.layout;
    }

    return frame;
};

uint32_t getInstanceID(InstanceIDAllocator &allocator, uint64_t key)
{
    auto it = allocator.idMapping.find(key);
    if (it != allocator.idMapping.end())
    {
        return it->second;
    }

    uint32_t id = allocator.nextID++;
    allocator.idMapping[key] = id;
    return id;
}