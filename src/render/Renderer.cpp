#include "Renderer.h"
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"
#include "Texture.h"
#include "Material.h"

/*
Thing not implementend:
Cleaning Image/Depth Stencil/Manually clean attachements
*/

void Renderer::createFramesData(uint32_t framesInFlightCount, const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
  auto logicalDevice = mContext->getLDevice().getLogicalDevice();
  auto physicalDevice = mContext->getPDeviceM().getPhysicalDevice();
  auto graphicsFamilyIndex = mContext->getPDeviceM().getIndices().graphicsFamily.value();

  mFrameHandler.createFramesData(logicalDevice, physicalDevice, graphicsFamilyIndex, framesInFlightCount);

  // Init Frame Descriptor set of Scene instance Buffer
  PipelineSetLayoutBuilder meshSSBO;
  meshSSBO.addDescriptor(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

  std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutBindings = {bindings, meshSSBO.descriptorSetLayoutsBindings};
  mFrameHandler.createFramesDescriptorSet(logicalDevice, layoutBindings);

  // Update/Store UBO
  mFrameHandler.writeFramesDescriptors(logicalDevice, 0);
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

void Renderer::initRenderInfrastructure(RenderPassType type, const RenderTargetConfig &cfg)
{
  std::cout << "Init Render Infrastructure" << std::endl;

  const auto &mLogDeviceM = mContext->getLDevice();
  const auto &mPhysDeviceM = mContext->getPDeviceM();
  const auto &mSwapChainM = mContext->getSwapChainManager();

  const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();
  // Onward is just hardcoded stuff not meant to be dynamic yet
  const VmaAllocator &allocator = mLogDeviceM.getVmaAllocator();

  if (cfg.type == RenderConfigType::Dynamic)
  {
    // Collect image views for dynamic rendering
    std::vector<VkImageView> colorViews = mGBuffers.collectColorViews(cfg.getClrAttachmentsID());

    // Configure per-frame rendering info
    createFramesDynamicRenderingInfo(type, cfg, colorViews, mGBuffers.getDepthImageView(),
                                     mSwapChainM.getSwapChainExtent());
  }
};

void Renderer::initRenderInfrastructure(RenderPassType type, const RenderPassConfig &cfg)
{
  std::cout << "Init Render Infrastructure" << std::endl;

  const auto &mLogDeviceM = mContext->getLDevice();
  const auto &mPhysDeviceM = mContext->getPDeviceM();
  const auto &mSwapChainM = mContext->getSwapChainManager();

  const VkDevice &device = mLogDeviceM.getLogicalDevice();

  const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();

  // Onward is just hardcoded stuff not meant to be dynamic yet
  const VmaAllocator &allocator = mLogDeviceM.getVmaAllocator();

  if (cfg.type == RenderConfigType::LegacyRenderPass)
  {

    mRenderPassM.createRenderPass(device, type, cfg);

    std::vector<VkImageView> views = mGBuffers.collectColorViews(cfg.getClrAttachmentsID());
    views.push_back(mGBuffers.getDepthImageView());

    mFrameHandler.completeFrameBuffers(device, views, mRenderPassM.getRenderPass(type), type,
                                       mSwapChainM.GetSwapChainImageViews(),
                                       mSwapChainM.getSwapChainExtent());
  }
};

// All the rest are frame ressourees
// Todo: This iss uploading which should be reworked once scene managment is redone
void Renderer::initRenderingRessources(Scene &scene, const AssetRegistry &registry, MaterialSystem &system)
{
  const VkDevice &device = mContext->getLDevice().getLogicalDevice();
  const VkPhysicalDevice &physDevice = mContext->getPDeviceM().getPhysicalDevice();
  const uint32_t indice = mContext->getPDeviceM().getIndices().graphicsFamily.value();

  mRScene.initInstanceBuffer(mGpuRegistry);
  // Create pipeline
  // Todo: automatically resolve this kind of stuff
  auto vf = VertexFormatRegistry::getStandardFormat();

  PipelineLayoutConfig sceneConfig{{mFrameHandler.getCurrentFrameData().mDescriptor.getDescriptorLat(0), {}, mFrameHandler.getCurrentFrameData().mDescriptor.getDescriptorLat(1)}, scene.sceneLayout.pushConstants};
  int pipelineEntryIndex = 0;
  auto layout = MaterialLayoutRegistry::Get(MaterialType::PBR);
  {
    // resolve materials globally
    auto &matDescriptors = system.materialDescriptor();
    int matLayoutIdx = matDescriptors.getOrCreateSetLayout(device, layout.descriptorSetLayoutsBindings);
    sceneConfig.descriptorSetLayouts[1] = matDescriptors.getDescriptorLat(matLayoutIdx);
    pipelineEntryIndex = requestPipeline(sceneConfig, vertPath, fragPath);
  }

  for (auto &node : scene.nodes)
  {
    const auto &mesh = registry.get(node.mesh);
    for (auto &mat : mesh->materialIds)
    {
      auto gpuMat = system.requestMaterial(mat);
      system.addMaterialInstance(gpuMat, RenderPassType::Forward, 0, pipelineEntryIndex);
    }
  }

  // This can't stay longterm
  // Get the light packet from the scene dynamically later
  // Todo: Result of Frames tied light
  LightPacket lights = scene.getLightPacket();
  for (int i = 0; i < mFrameHandler.getFramesCount(); i++)
  {
    uint8_t *dirLightMapping = static_cast<uint8_t *>(mFrameHandler.getCurrentFrameData().mDirLightMapping);
    uint8_t *ptLightMapping = static_cast<uint8_t *>(mFrameHandler.getCurrentFrameData().mPtLightMapping);
    // lights.directionalCount
    int count = 10;
    if (lights.directionalCount > 0)
    {
      memcpy(
          dirLightMapping,
          lights.directionalLights.data(),
          lights.directionalCount * lights.dirLigthSize);
      // lights.dirLigthSize * count);
    }

    memcpy(dirLightMapping + (lights.dirLigthSize * count),
           &lights.directionalCount,
           sizeof(lights.directionalCount));

    if (lights.pointLightSize > 0)
    {
      memcpy(
          ptLightMapping,
          lights.pointLights.data(),
          lights.pointCount * lights.pointLightSize);
      // lights.pointLightSize * count);
    }

    memcpy(ptLightMapping + (lights.pointLightSize * count),
           &lights.pointCount,
           sizeof(lights.pointCount));

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

void Renderer::updateRenderingScene(const VisibilityFrame &vFrame, const AssetRegistry &registry, MaterialSystem &matSystem)
{
  // Todo:

  mRScene.updateFrameSync(*mContext, vFrame, mGpuRegistry, registry, matSystem, mFrameHandler.getCurrentFrameIndex());
  passes.clear();
  // Shadow pass
  {
    // RenderPassFrame &shadow = frameGraph.shadowPass;
    // shadow.type = RenderPassType::Shadow;
    // buildPassFrame(shadow, scene, pipelineCache);
  }
  // Forward pass
  {
    RenderPassFrame forward;
    forward.type = RenderPassType::Forward;
    buildPassFrame(forward, mRScene, matSystem);
    passes.push_back(std::move(forward));
  }
}

void Renderer::deinitSceneRessources()
{
  const auto allocator = mContext->getLDevice().getVmaAllocator();
  const auto device = mContext->getLDevice().getLogicalDevice();

  // Below is more destroy Renderer than anything else
  mGBuffers.destroy(device, allocator);
  mGpuRegistry.clearAll();
  for (int i = 0; i < mContext->mSwapChainM.GetSwapChainImageViews().size(); i++)
  {
    auto &frameData = mFrameHandler.getCurrentFrameData();
    frameData.mFramebuffer.destroyFramebuffers(device);
    mFrameHandler.advanceFrame();
  }

  mFrameHandler.destroyFramesData(device);

  mRenderPassM.destroyAll(device);
  mPipelineM.destroy(device);
}

int Renderer::requestPipeline(const PipelineLayoutConfig &config,
                              const std::string &vertexPath,
                              const std::string &fragmentPath)
{

  const VkDevice &device = mContext->getLDevice().getLogicalDevice();

  // For dynamic rendering
  // slice = std::vector<VkFormat>();
  // Slice asssuming  attachment last attachment is the depth

  // Pipeline should obviously not be constructed here
  PipelineBuilder builder;
  builder.setShaders({vertexPath, fragmentPath})
      .setInputConfig({.vertexFormat = VertexFormatRegistry::getStandardFormat()});

  if (mUseDynamic)
  {
    auto attachments = mGBuffers.getAllFormats();
    attachments.insert(attachments.begin(), mContext->getSwapChainManager().getSwapChainImageFormat().format);
    std::vector<VkFormat> clrAttach(attachments.begin(), attachments.end() - 1);

    builder.setDynamicRenderPass(clrAttach, attachments.back());
  }
  else
  {
    builder.setRenderPass(mRenderPassM.getRenderPass(RenderPassType::Forward));
  }

  builder.setUniform(config);

  return mPipelineM.createPipelineWithBuilder(device, builder);
};

void Renderer::createFramesDynamicRenderingInfo(RenderPassType type, const RenderTargetConfig &cfg,
                                                const std::vector<VkImageView> &gbufferViews,
                                                VkImageView depthView, const VkExtent2D swapChainExtent)
{
  const uint16_t index = static_cast<uint16_t>(type);
  auto &renderColorInfos = mDynamicPassesInfo[index].colorAttachments;
  auto &renderDepthInfo = mDynamicPassesInfo[index].depthAttachment;
  auto &renderInfo = mDynamicPassesInfo[index].info;
  mDynamicPassesConfig[index] = cfg;

  renderColorInfos.clear();

  // SwapChain image
  VkRenderingAttachmentInfo swapchainColor;
  swapchainColor = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  // swapchainColor.imageView = swapChainView; //Notes: This is set dynamically during render

  swapchainColor.imageLayout = cfg.attachments[0].finalLayout; // target layout for rendering
  swapchainColor.loadOp = cfg.attachments[0].loadOp;
  swapchainColor.storeOp = cfg.attachments[0].storeOp;
  swapchainColor.clearValue = {{0.2f, 0.2f, 0.2f, 1.0f}};
  renderColorInfos.push_back(swapchainColor);

  for (size_t i = 0; i < gbufferViews.size(); ++i)
  {
    VkRenderingAttachmentInfo colorInfo{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    colorInfo.imageView = gbufferViews[i];
    colorInfo.imageLayout = cfg.attachments[i + 1].finalLayout; // target layout for rendering
    colorInfo.loadOp = cfg.attachments[i + 1].loadOp;
    colorInfo.storeOp = cfg.attachments[i + 1].storeOp;
    // colorInfo.clearValue =  set per-pass per-frame
    renderColorInfos.push_back(colorInfo);
  }

  if (cfg.enableDepth && depthView != VK_NULL_HANDLE)
  {
    renderDepthInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    renderDepthInfo.imageView = depthView;
    renderDepthInfo.imageLayout = cfg.attachments.back().finalLayout;
    renderDepthInfo.loadOp = cfg.attachments.back().loadOp;
    renderDepthInfo.storeOp = cfg.attachments.back().storeOp;
    renderDepthInfo.clearValue = {1.0f, 0}; // Important...
  }
  // fill depth load/store...
  renderInfo = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                .renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
                .layerCount = 1,
                .colorAttachmentCount = static_cast<uint32_t>(renderColorInfos.size()),
                .pColorAttachments = renderColorInfos.data(),
                .pDepthAttachment = cfg.enableDepth ? &renderDepthInfo : nullptr};
  // Todo: set renderArea / layerCount / viewMask as needed
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

  // Update Camera and global UBO
  // mFrameHandler.updateUniformBuffers(sceneData.viewproj);
  memcpy(mFrameHandler.getCurrentFrameData().mCameraMapping, &sceneData, sizeof(SceneData));

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
  VkExtent2D extent = mContext->mSwapChainM.getSwapChainExtent();
  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();
  VkCommandBuffer command = frameRess.mCommandPool.get();
  uint32_t renderPassIndex = static_cast<uint32_t>(type);

  if (mUseDynamic)
  {
    // Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL for rendering

    const auto &images = mContext->mSwapChainM.GetSwapChainImages();
    auto transObj = vkUtils::Texture::makeTransition(images[mIndexImage], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    transObj.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    transObj.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    transObj.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkUtils::Texture::recordImageMemoryBarrier(command, transObj);

    // Todo: Pretty hard to do and reaad +  swapchain need to always be first
    mDynamicPassesInfo[renderPassIndex].colorAttachments[0].imageView = mContext->getSwapChainManager().GetSwapChainImageViews()[mFrameHandler.getCurrentFrameIndex()];
    VkRenderingInfo renderInfo = mDynamicPassesInfo[renderPassIndex].info;
    vkCmdBeginRendering(command, &renderInfo);
  }
  else
  {
    mRenderPassM.startPass(renderPassIndex, command,
                           frameRess.mFramebuffer.getFramebuffers(renderPassIndex), extent);
  }

  // Setup viewport / scissor
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.extent = extent;
  vkCmdSetScissor(command, 0, 1, &scissor);
};

void Renderer::endPass(RenderPassType type)
{
  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();

  if (mUseDynamic)
  {
    FrameResources &frameRess = mFrameHandler.getCurrentFrameData();
    auto &commandPoolM = frameRess.mCommandPool;
    const VkCommandBuffer command = commandPoolM.get();

    const auto &images = mContext->mSwapChainM.GetSwapChainImages();

    vkCmdEndRendering(frameRess.mCommandPool.get());

    auto transObjb = vkUtils::Texture::makeTransition(images[mIndexImage],
                                                      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);
    transObjb.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    transObjb.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    transObjb.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    transObjb.dstAccessMask = 0;

    vkUtils::Texture::recordImageMemoryBarrier(command, transObjb);
  }
  else
  {
    mRenderPassM.endPass(frameRess.mCommandPool.get());
  }
};

// Handle non Material object
void Renderer::drawFrame(const SceneData &sceneData, const RenderPassFrame &pass, const DescriptorManager &materialDescriptor)
{
  /*
  vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
  instanceCount: Used for instanced rendering, use 1 if you're not doing that.
  firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
  firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.

  for each view {
bind global resourcees          // set 0

for each shader {
  bind shader pipeline
  for each material {
    bind material resources  // sets 1 maybe 2
  }
}
}
  */

  // 0 is not quite right
  // I m also not quite sure how location is determined, just through the attributes description
  // 1 interleaved buffer here

  // MESH DRAWING

  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();
  VkCommandBuffer cmd = frameRess.mCommandPool.get();

  // Draw all queues
  for (const PassQueue &queue : pass.queues)
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipeline(queue.pipelineIndex));
    for (auto *draw : queue.drawCalls)
    {

      GPUBufferRef vtxGpu = mGpuRegistry.getBuffer(draw->vtxBuffer);
      GPUBufferRef idxGPU = mGpuRegistry.getBuffer(draw->idxBuffer);
      auto *materialGpu = mGpuRegistry.getMaterial(draw->materialGPU);

      // Bind descriptors
      std::vector<VkDescriptorSet> sets = {
          frameRess.mDescriptor.getSet(0),
          materialDescriptor.getSet(materialGpu->descriptorIndex),
          frameRess.mDescriptor.getSet(1)};

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              mPipelineM.getPipelineLayout(queue.pipelineIndex), 0,
                              sets.size(), sets.data(),
                              0, nullptr);

      // Bind vertex
      VkDeviceSize offsets[] = {0};
      auto vtxBuffer = vtxGpu.buffer->getBuffer();
      auto idxBuffer = idxGPU.buffer->getBuffer();

      vkCmdBindVertexBuffers(cmd, 0, 1, &vtxBuffer, offsets);
      vkCmdBindIndexBuffer(cmd, idxBuffer, 0, VK_INDEX_TYPE_UINT32);

      // Push constants
      vkCmdPushConstants(cmd, mPipelineM.getPipelineLayout(queue.pipelineIndex),
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &sceneData.viewproj);

      // Draw
      for (auto &instRange : draw->instanceRanges)
      {
        vkCmdDrawIndexed(cmd, draw->indexCount, instRange.count, draw->indexOffset, 0, instRange.first);
      }
    }

    // Fake multi Viewport is basically this
    /*
    viewport.width = static_cast<float>(frameExtent.width);
    viewport.height = static_cast<float>(frameExtent.height/2);
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);
    */
  }
};
