#include "Renderer.h"
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"
#include "Texture.h"
#include "Material.h"

/*
Thing not implementend:
Cleaning Image/Depth Stencil/Manually clean attachements
*/

void Renderer::createFramesData(uint32_t framesInFlightCount)
{
  auto logicalDevice = mContext->getLDevice().getLogicalDevice();
  auto physicalDevice = mContext->getPDeviceM().getPhysicalDevice();
  auto graphicsFamilyIndex = mContext->getPDeviceM().getIndices().graphicsFamily.value();

  mFrameHandler.createFramesData(logicalDevice, physicalDevice, graphicsFamilyIndex, framesInFlightCount);

  mFrameHandler.createShadowTextures(mContext->getLDevice(), physicalDevice, graphicsFamilyIndex);
}

void Renderer::initAllGbuffers(std::vector<VkFormat> gbufferFormats, bool depth)
{
  const auto &mLogDeviceM = mContext->getLDevice();
  const auto &mPhysDeviceM = mContext->getPDeviceM();
  const auto &mSwapChainM = mContext->getSwapChainManager();

  const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();
  const VmaAllocator &allocator = mLogDeviceM.getVmaAllocator();

  mGBuffers.init(mSwapChainM.getSwapChainExtent());

  mGBuffers.createGBuffers(mLogDeviceM, mPhysDeviceM, gbufferFormats, allocator);
  // if (depth)
  //{

  mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat, allocator);
  //}
};

// All the rest are frame ressourees
// Todo: This iss uploading which should be reworked once scene managment is redone
void Renderer::initRenderingRessources(Scene &scene, const AssetRegistry &registry, MaterialSystem &system, std::vector<VkPushConstantRange> pushConstants)
{
  const VkDevice &device = mContext->getLDevice().getLogicalDevice();
  const VkPhysicalDevice &physDevice = mContext->getPDeviceM().getPhysicalDevice();
  const uint32_t indice = mContext->getPDeviceM().getIndices().graphicsFamily.value();

  mRScene.initInstanceBuffer(mGpuRegistry);
  // Create pipeline
  // Todo: automatically resolve this kind of stuff
  auto vf = VertexFormatRegistry::getStandardFormat();

  PipelineLayoutConfig sceneConfig{{mFrameHandler.getCurrentFrameData().mDescriptor.getDescriptorLat(0), mFrameHandler.getCurrentFrameData().mDescriptor.getDescriptorLat(1)}, pushConstants};

  int pipelineEntryIndex = 0;
  int pipelineEntryIndexShadow = 0;

  // TODO: Important
  const std::string vertPath = "./ressources/shaders/vert.spv";
  const std::string fragPath = "./ressources/shaders/frag.spv";

  auto layout = MaterialLayoutRegistry::Get(MaterialType::PBR);
  {
    // resolve materials globally
    auto &matDescriptors = system.materialDescriptor();
    int matLayoutIdx = matDescriptors.getOrCreateSetLayout(device, layout.bindings);
    sceneConfig.descriptorSetLayouts.push_back(matDescriptors.getDescriptorLat(matLayoutIdx));

    for (const auto &passes : mPassesHandler.getExecutions())
    {
      if (passes == RenderPassType::Forward)
      {
        pipelineEntryIndex = requestPipeline(sceneConfig, vertPath, fragPath, passes);
      }
      else
      {
        auto shadowPath = cico::fs::shaders() / "shadow.spv";
        pipelineEntryIndexShadow = requestPipeline(sceneConfig, shadowPath, "", RenderPassType::Shadow);
      }
    }
  }

  // Todo: Current state force the duplication of Instance per material which is a problematic but unforeseen thing
  for (auto &node : scene.nodes)
  {
    const auto &mesh = registry.get(node.mesh);
    for (auto &mat : mesh->materialIds)
    {
      auto gpuMat = system.requestMaterial(mat);
      system.addMaterialInstance(gpuMat, RenderPassType::Forward, 0, pipelineEntryIndex);
      system.addMaterialInstance(gpuMat, RenderPassType::Shadow, 0, pipelineEntryIndexShadow);
    }
  }

  // This can't stay longterm
  // Get the light packet from the scene dynamically later
  // Todo: Result of Frames tied light
  LightPacket lights = scene.getLightPacket();
  scene.updateShadows();
  ShadowPacket shadow = scene.getShadowPacket();

  for (int i = 0; i < mFrameHandler.getFramesCount(); i++)
  {

    uint8_t *shadowMapping = static_cast<uint8_t *>(mFrameHandler.getCurrentFrameData().mShadowMapping);

    int frameIndex = mFrameHandler.getCurrentFrameIndex();
    auto istBufferRef = mGpuRegistry.getBuffer(mRScene.instanceBuffer, frameIndex);
    auto &descriptor = mFrameHandler.getCurrentFrameData().mDescriptor;

    VkDescriptorBufferInfo istSSBO = istBufferRef.buffer->getDescriptor();

    std::vector<VkWriteDescriptorSet> writes = {
        vkUtils::Descriptor::makeWriteDescriptor(descriptor.getSet(1), 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &istSSBO),
    };

    descriptor.updateDescriptorSet(device, writes);

    mFrameHandler.advanceFrame();
  }

  std::cout << "Ressourcess uploaded" << std::endl;
  std::cout << "Scene Ressources Initialized" << std::endl;
};

void Renderer::createDescriptorSet(DescriptorScope scope, std::vector<VkDescriptorSetLayoutBinding> &bindings)
{

  const auto device = mContext->getLDevice().getLogicalDevice();
  for (int i = 0; i < mFrameHandler.getFramesCount(); i++)
  {
    switch (scope)
    {
    case DescriptorScope::Global:
    {
      mFrameHandler.createFrameDescriptor(device, bindings, mFrameHandler.getCurrentFrameIndex(), 0);
      break;
    }
    case DescriptorScope::Instances:
    {
      mFrameHandler.createFrameDescriptor(device, bindings, mFrameHandler.getCurrentFrameIndex(), 1);
      break;
    }
    default:
      break;
    }
    mFrameHandler.advanceFrame();
  }
}

// TODO: Better name to reflect it only work with some of the DescriptorScope
void Renderer::setupFramesDescriptor(DescriptorScope scope, std::vector<ResourceLink> &links)
{
  const auto device = mContext->getLDevice().getLogicalDevice();
  uint32_t framesCount = mFrameHandler.getFramesCount();
  int usedSet = static_cast<uint32_t>(scope);

  if (usedSet > 1)
  { // Passes or Material Descriptor
    return;
  }
  // We ensure the Registry has one big buffer for each link's name which force some alignment stuff
  // Ideally, i should figure out a way to always have struct alignas(64) here 64 is the mniALignemnt

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(mContext->getPDeviceM().getPhysicalDevice(), &props);

  size_t minUboAlign = props.limits.minUniformBufferOffsetAlignment;
  size_t minSsboAlign = props.limits.minStorageBufferOffsetAlignment;

  auto alignUp = [](size_t value, size_t alignment) -> size_t
  {
    return (value + alignment - 1) & ~(alignment - 1);
  };

  // Loop over all links and create aligned big buffers
  for (auto &[binding, desc] : links)
  {
    size_t baseSize = desc.size; // size of one object/frame

    // Decide proper alignment for this buffer
    size_t requiredAlignment = (desc.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) ? minSsboAlign : minUboAlign;
    size_t alignedSize = alignUp(baseSize, requiredAlignment);
    desc.size = alignedSize * framesCount;

    auto bufferKey = mGpuRegistry.addBuffer(AssetID<void>{}, desc);

    for (int i = 0; i < framesCount; i++)
    {
      GPUResourceRegistry::BufferAllocation allocation{
          .offset = i * alignedSize,
          .size = baseSize};

      mGpuRegistry.allocateInBuffer(bufferKey, allocation);
    }
  }

  for (int i = 0; i < mFrameHandler.getFramesCount(); i++)
  {
    auto &frameData = mFrameHandler.getCurrentFrameData();
    const auto frameNum = mFrameHandler.getCurrentFrameIndex();

    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> bufferInfos;

    writes.reserve(links.size());
    bufferInfos.reserve(links.size());

    for (auto &[binding, desc] : links)
    {
      auto info = mGpuRegistry.getBuffer(desc.name, frameNum);

      bufferInfos.push_back(info.getDescriptor());
      VkDescriptorType type = (desc.usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
                                  ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                                  : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      writes.push_back(vkUtils::Descriptor::
                           makeWriteDescriptor(frameData.mDescriptor.getSet(usedSet), binding,
                                               type, &bufferInfos.back()));
    }

    frameData.mDescriptor.updateDescriptorSet(device, writes);
    mFrameHandler.advanceFrame();
  }
}

void Renderer::updateBuffer(const std::string &name, const void *data, size_t size)
{
  uint32_t currentFrame = mFrameHandler.getCurrentFrameIndex();

  auto info = mGpuRegistry.getBuffer(name, currentFrame);
  info.uploadData(*mContext, data, size);
}

void Renderer::updateRenderingScene(const VisibilityFrame &vFrame, const AssetRegistry &registry, MaterialSystem &matSystem)
{
  mRScene.updateFrameSync(*mContext, vFrame, mGpuRegistry, registry, matSystem, mFrameHandler.getCurrentFrameIndex());
  for (const auto &passes : mPassesHandler.getExecutions())
  {
    auto &forward = mPassesHandler.getBackend(passes);
    forward.frames.type = passes;
    buildPassFrame(forward.frames, mRScene, matSystem);
  }
}

void Renderer::deinitSceneRessources()
{
  const auto allocator = mContext->getLDevice().getVmaAllocator();
  const auto device = mContext->getLDevice().getLogicalDevice();

  // Below is more destroy Renderer than anything else
  mGBuffers.destroy(device, allocator);
  mGpuRegistry.clearAll();

  mFrameHandler.destroyFramesData(device);

  mPassesHandler.destroyRessources(device);
  mPipelineM.destroy(device);
}

int Renderer::requestPipeline(const PipelineLayoutConfig &config,
                              const std::string &vertexPath,
                              const std::string &fragmentPath, RenderPassType type)
{

  const VkDevice &device = mContext->getLDevice().getLogicalDevice();

  // For dynamic rendering
  // slice = std::vector<VkFormat>();
  // Slice asssuming  attachment last attachment is the depth

  // Pipeline should obviously not be constructed here
  PipelineBuilder builder;
  builder.setShaders({vertexPath, fragmentPath})
      .setInputConfig({.vertexFormat = VertexFormatRegistry::getStandardFormat()});

  auto &pass = mPassesHandler.getBackend(type);
  std::vector<VkFormat> colorFormats;
  VkFormat depthFormat;
  for (const auto &subpass : pass.config.subpasses)
  {

    for (const auto &colotAttachment : subpass.colorAttachments)
    {
      const auto &binding = pass.config.attachments[colotAttachment.index];
      if (binding.config.role != AttachmentConfig::Role::Depth)
      {
        colorFormats.emplace_back(binding.config.format);
      }
    }
    if (subpass.depthAttachment.has_value())
    {
      depthFormat = pass.config.attachments[subpass.depthAttachment.value().index].config.format;
      ;
    }
  }

  if (pass.isDynamic())
  {
    builder.setDynamicRenderPass(colorFormats, depthFormat);
  }
  else
  {
    builder.setRenderPass(mPassesHandler.getBackend(type).renderPassLegacy.getRenderPass(0));
  }

  builder.setUniform(config);

  return mPipelineM.createPipelineWithBuilder(device, builder);
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// Drawing Loop Functions//////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
// Semaphore and command buffer tied to frames

void Renderer::beginFrame(const SceneData &sceneData, GLFWwindow *window)
{

  // Handle fetching
  VkDevice device = mContext->mLogDeviceM.getLogicalDevice();
  SwapChainManager &chain = mContext->getSwapChainManager();

  // Todo: Not sure about exposing this as non const
  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();

  // In a real app we could do other stuff while waiting on Fence
  // Or maybe multiple submitted task with each their own fence they get so that they can move unto a specific task
  // I suppose it woud be on another thread
  frameRess.mSyncObjects.waitFenceSignal(device);

  bool resultAcq = chain.aquireNextImage(device,
                                         frameRess.mSyncObjects.getImageAvailableSemaphore(),
                                         mIndexImage);

  if (!resultAcq)
  {
    // Todo: Should be in swapchain?
    // recreateSwapChain(device, window);
    // return;
  }

  frameRess.mSyncObjects.resetFence(device);

  // Handle fetching
  // Todo: Not sure about exposing this
  // Create shorthand from frame to record
  mFrameHandler.getCurrentFrameData().mCommandPool.beginRecord();
}

void Renderer::endFrame(bool framebufferResized)
{
  SwapChainManager &chain = mContext->getSwapChainManager();
  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();
  frameRess.mCommandPool.endRecord();

  // Submit Info set up
  mContext->mLogDeviceM.submitFrameToGQueue(
      frameRess.mCommandPool.get(),
      frameRess.mSyncObjects.getImageAvailableSemaphore(),
      frameRess.mSyncObjects.getRenderFinishedSemaphore(),
      frameRess.mSyncObjects.getInFlightFence());
  VkResult result = mContext->mLogDeviceM.presentImage(chain.GetChain(),
                                                       frameRess.mSyncObjects.getRenderFinishedSemaphore(),
                                                       mIndexImage);

  // Recreate the Swap Chain if suboptimal
  // Todo: This checck could be directly hadnled in present Image if presentImage was in SwapcHain
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
  {
    framebufferResized = false;
    // recreateSwapChain(device, window);
  }
  else if (result != VK_SUCCESS)
  {
    throw std::runtime_error("failed to present swap chain image!");
  }

  // Go to the next index
  mFrameHandler.advanceFrame();
};

void Renderer::beginPass(RenderPassType type)
{
  auto &frame = mFrameHandler.getCurrentFrameData();
  VkCommandBuffer cmd = frame.mCommandPool.get();
  mPassesHandler.beginPass(type, cmd);
};

void Renderer::endPass(RenderPassType type)
{
  auto &frame = mFrameHandler.getCurrentFrameData();
  VkCommandBuffer cmd = frame.mCommandPool.get();
  mPassesHandler.endPass(type, cmd);
};

// Handle non Material object
void Renderer::drawFrame(RenderPassType type, const SceneData &sceneData, const DescriptorManager &materialDescriptor)
{
  /*
  vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
  instanceCount: Used for instanced rendering, use 1 if you're not doing that.
  firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
  firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
  */

  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();
  VkCommandBuffer cmd = frameRess.mCommandPool.get();

  // Draw all queues
  auto &pass = mPassesHandler.getBackend(type);
  auto& currentFrameSets = pass.scopedSets[mFrameHandler.getCurrentFrameIndex()];
  for (const PassQueue &queue : pass.frames.queues)
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipeline(queue.pipelineIndex));
    for (auto *draw : queue.drawCalls)
    {

      GPUBufferRef vtxGpu = mGpuRegistry.getBuffer(draw->vtxBuffer);
      GPUBufferRef idxGPU = mGpuRegistry.getBuffer(draw->idxBuffer);
      auto *materialGpu = mGpuRegistry.getMaterial(draw->materialGPU);

      // Bind descriptors
      std::array<VkDescriptorSet, 4> sets;
      uint32_t count = 0;

      // We check the pass's mask to see which slots to fill
      
          
      if (pass.activeScopesMask & (1 << 0)) sets[count++] = frameRess.mDescriptor.getSet(0);
      if (pass.activeScopesMask & (1 << 1)) sets[count++] = frameRess.mDescriptor.getSet(1);
      if (pass.activeScopesMask & (1 << 2)) sets[count++] = materialDescriptor.getSet(materialGpu->descriptorIndex);
      if (pass.activeScopesMask & (1 << 3)) sets[count++] = mPassesHandler.getDescriptors().getSet(pass.descriptorSetIndex);

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              mPipelineM.getPipelineLayout(queue.pipelineIndex), 0,
                              count, sets.data(),
                              0, nullptr);

      // Bind vertex
      VkDeviceSize offsets[] = {0};
      auto vtxBuffer = vtxGpu.buffer->getBuffer();
      auto idxBuffer = idxGPU.buffer->getBuffer();

      vkCmdBindVertexBuffers(cmd, 0, 1, &vtxBuffer, offsets);
      vkCmdBindIndexBuffer(cmd, idxBuffer, 0, VK_INDEX_TYPE_UINT32);
      vkCmdPushConstants(cmd, mPipelineM.getPipelineLayout(queue.pipelineIndex),
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &sceneData.viewproj);

      // Draw
      for (auto &instRange : draw->instanceRanges)
      {
        vkCmdDrawIndexed(cmd, draw->indexCount, instRange.count, draw->indexOffset, 0, instRange.first);
      }
    }
  }
};
