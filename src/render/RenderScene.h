#pragma once
#include "Renderer.h"
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"
#include "Texture.h"
#include "Material.h"
#include "MaterialSystem.h"
#include "Scene.h"

struct PassQueue
{
  uint32_t pipelineIndex;
  std::vector<Drawable *> drawCalls;
};

struct RenderPassFrame
{
  RenderPassType type;
  std::vector<VkDescriptorSet> passSet;
  std::vector<PassQueue> queues;
};

class RenderScene
{
public:
  RenderScene() = default;
  ~RenderScene() = default;

  void initInstanceBuffer(
      GPUResourceRegistry &registry)
  {
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    constexpr uint32_t TRANSFORM_BUFFER_KEY = 0;

    BufferDesc description;
    description.updatePolicy = BufferUpdatePolicy::Dynamic;
    description.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    description.size = MAX_INSTANCES * MAX_FRAMES_IN_FLIGHT;
    description.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    description.ssbo = true;
    description.stride = sizeof(InstanceTransform);
    AssetID<void> instanceBufferId(TRANSFORM_BUFFER_KEY);
    // RenderScene might as well own this
    instanceBuffer = registry.addBuffer(instanceBufferId, description, 0);
    // Handle the allocations
    instanceBuffer = registry.addBuffer(instanceBufferId, description, MAX_INSTANCES);
    instanceBuffer = registry.addBuffer(instanceBufferId, description, MAX_INSTANCES * 2);
  }
  BufferKey instanceBuffer;
  std::unordered_map<uint64_t, Drawable> drawables;
  InstanceIDAllocator mInstanceAllocator;

  uint32_t getOrAssignInstance(uint64_t instanceKey)
  {
    getInstanceID(mInstanceAllocator, instanceKey);
  };

  static constexpr uint32_t MAX_INSTANCES = 1024;
};

/*
// updateFrameSync():
// - may allocate GPU resources
// - may reallocate instance buffers (not currently)
// - will uploads per-frame instance data (everytime when instancing exist or with dynamic meshes)
// - must be called before rendering
*/

void updateFrameSync(
    VulkanContext &ctx,
    RenderScene &scene,
    const VisibilityFrame &frame,
    GPUResourceRegistry &registry,
    const AssetRegistry &cpuRegistry,
    MaterialSystem matSystem, uint32_t currFrame);

void buildPassFrame(
    RenderPassFrame &outFrame,
    RenderScene &scene,
    MaterialSystem &materialSystem);
