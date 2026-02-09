#pragma once
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"
#include "Scene.h" //Visibility frame among other
#include "Drawable.h"

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

class MaterialSystem;

class RenderScene
{
public:
  RenderScene() = default;
  ~RenderScene() = default;

  static constexpr uint32_t MAX_INSTANCES = 1024;

  /*
  struct Config
  {
    bool useFrustumCulling = true;
    bool useInstancing = true;
    uint32_t maxInstances = 65536;
    Framein flight ? trasnform key ?
    BufferUsage instanceBufferUsage = BufferUsage::Storage;
  };
  */

  void initInstanceBuffer(
      GPUResourceRegistry &registry)
  {
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    constexpr uint32_t TRANSFORM_BUFFER_KEY = 0;

    uint32_t bufferSize = MAX_INSTANCES * sizeof(InstanceTransform);
    BufferDesc description;
    description.updatePolicy = BufferUpdatePolicy::Dynamic;
    description.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    description.size = bufferSize * MAX_FRAMES_IN_FLIGHT ;
    description.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    description.ssbo = true;
    //description.stride = sizeof(InstanceTransform);
    AssetID<void> instanceBufferId(TRANSFORM_BUFFER_KEY);
    // RenderScene might as well own this
    instanceBuffer = registry.addBuffer(instanceBufferId, description);
    // Handle the allocations
    GPUResourceRegistry::BufferAllocation allocation{.offset = 0, .size = bufferSize};
    registry.allocateInBuffer(instanceBuffer, allocation);
    registry.allocateInBuffer(instanceBuffer, allocation);
    registry.allocateInBuffer(instanceBuffer, allocation);
  }

  BufferKey instanceBuffer;
  std::unordered_map<uint64_t, Drawable> drawables;
  InstanceIDAllocator mInstanceAllocator;

  uint32_t getOrAssignInstance(uint64_t instanceKey)
  {
    // Downgrade as 2^32 instances seem reasonable
    return getInstanceID(mInstanceAllocator, instanceKey);
  };

  void updateFrameSync(
      VulkanContext &ctx,
      const VisibilityFrame &frame,
      GPUResourceRegistry &registry,
      const AssetRegistry &cpuRegistry,
      MaterialSystem &matSystem, uint32_t currFrame);
};

/*
// updateFrameSync():
// - may allocate GPU resources
// - may reallocate instance buffers (not currently)
// - will uploads per-frame instance data (everytime when instancing exist or with dynamic meshes)
// - must be called before rendering
*/

void buildPassFrame(
    RenderPassFrame &outFrame,
    RenderScene &scene,
    MaterialSystem &materialSystem);
