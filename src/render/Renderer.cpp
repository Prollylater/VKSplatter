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
  mFrameHandler.createShadowTextures(mContext->getLDevice(), physicalDevice, graphicsFamilyIndex);
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
  int pipelineEntryIndexShadow = 0;

  auto layout = MaterialLayoutRegistry::Get(MaterialType::PBR);
  {
    // resolve materials globally
    auto &matDescriptors = system.materialDescriptor();
    int matLayoutIdx = matDescriptors.getOrCreateSetLayout(device, layout.descriptorSetLayoutsBindings);
    sceneConfig.descriptorSetLayouts[1] = matDescriptors.getDescriptorLat(matLayoutIdx);
    pipelineEntryIndex = requestPipeline(sceneConfig, vertPath, fragPath, RenderPassType::Forward);
    // Nearly the same
    auto shadowPath = cico::fs::shaders() / "shadowv.spv";
    pipelineEntryIndexShadow = requestPipeline(sceneConfig, shadowPath, "", RenderPassType::Shadow);
  }

  // Current state force the duplication of Instance per material which is a problematic but unforeseen thing
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
  mRScene.updateFrameSync(*mContext, vFrame, mGpuRegistry, registry, matSystem, mFrameHandler.getCurrentFrameIndex());
  // Shadow pass
  {
    //auto &shadow = mPassesHandler.getBackend(RenderPassType::Shadow);
    //shadow.frames.type = RenderPassType::Shadow;
    //buildPassFrame(shadow.frames, mRScene, matSystem);
  }
  // Forward pass
  {
    auto &forward = mPassesHandler.getBackend(RenderPassType::Forward);
    forward.frames.type = RenderPassType::Forward;
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

  if (mUseDynamic)
  {
    if (type != RenderPassType::Shadow)
    {
      // Hardcode,
      // Here we fetch format instead of passing them
      // getAllFormats is also a bad idea since it doesn't match what we actually want
      // PassesHandler should handle
      auto attachments = mGBuffers.getAllFormats();
      attachments.insert(attachments.begin(), mContext->getSwapChainManager().getSwapChainImageFormat().format);
      std::vector<VkFormat> clrAttach(attachments.begin(), attachments.end() - 1);
      builder.setDynamicRenderPass(clrAttach, attachments.back());
    }
    else
    {
      auto attachments = mGBuffers.getAllFormats();

      builder.setDynamicRenderPass({}, attachments.back());
    }
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
  auto &frame = mFrameHandler.getCurrentFrameData();
  VkCommandBuffer cmd = frame.mCommandPool.get();
  mPassesHandler.beginPass(type, cmd);
  /*
  // Todo :  Handle the proper transition for each frame through passesHandler
  if (mUseDynamic)
  {
    // Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL for rendering

    const auto &images = mContext->mSwapChainM.GetSwapChainImages();
    auto transObj = vkUtils::Texture::makeTransition(images[mIndexImage], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    transObj.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    transObj.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    transObj.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    vkUtils::Texture::recordImageMemoryBarrier(command, transObj);

    if (type == RenderPassType::Shadow)
    {
      auto image = frameRess.cascadePoolArray.getImage();

      auto transObj = vkUtils::Texture::makeTransition(
          image,
          VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
          VK_IMAGE_ASPECT_DEPTH_BIT);
      // PRepare the correct one for next passes
      transObj.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      transObj.dstStageMask =
          VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      transObj.dstAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

      vkUtils::Texture::recordImageMemoryBarrier(command, transObj);

      extent.height = 1024;
      extent.width = 1024;
    }

    // Todo: Pretty hard to do and reaad +  swapchain need to always be first

    mPassesHandler.updateDynamicRenderingInfo(type, extent);

    VkRenderingInfo renderInfo = mPassesHandler.getBackend(type).dynamicInfo.info;
    vkCmdBeginRendering(command, &renderInfo);
  }
  else
  {
    auto &pass = mPassesHandler.getBackend(type);
    // Todo: If mFrameHandler is to stay with pass, i might hide mFrameHandler.getCurrentFrameIndex() through a more direct start pass
    pass.renderPassLegacy.startPass(0, command, pass.frameBuffers.getFramebuffers(mFrameHandler.getCurrentFrameIndex()), extent);
  }

  if (type == RenderPassType::Shadow)
  {
    extent.height = 1024;
    extent.width = 1024;
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
    */

};

void Renderer::endPass(RenderPassType type)
{
  auto &frame = mFrameHandler.getCurrentFrameData();
  VkCommandBuffer cmd = frame.mCommandPool.get();
  mPassesHandler.endPass(type, cmd);
  /*
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
    auto &pass = mPassesHandler.getBackend(type);
    pass.renderPassLegacy.endPass(frameRess.mCommandPool.get());
  }*/
};

// Handle non Material object
void Renderer::drawFrame(RenderPassType type, const SceneData &sceneData, const DescriptorManager &materialDescriptor)
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
  auto &pass = mPassesHandler.getBackend(type);
  for (const PassQueue &queue : pass.frames.queues)
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
