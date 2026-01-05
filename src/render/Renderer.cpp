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
  auto logicalDevice = mContext->getLogicalDeviceManager().getLogicalDevice();
  auto physicalDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
  auto graphicsFamilyIndex = mContext->getPhysicalDeviceManager().getIndices().graphicsFamily.value();

  mFrameHandler.createFramesData(logicalDevice, physicalDevice, graphicsFamilyIndex, framesInFlightCount);

  // Init Frame Descriptor set of Scene
  // scene.sceneLayout.descriptorSetLayouts}; Could send the Camera directly here
  std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutBindings = {bindings};
  mFrameHandler.createFramesDescriptorSet(logicalDevice, layoutBindings);

  // Update/Store UBO
  mFrameHandler.writeFramesDescriptors(logicalDevice, 0);
  mFrameHandler.updateUniformBuffers(mContext->getSwapChainManager().getSwapChainExtent());
}

void Renderer::initAllGbuffers(std::vector<VkFormat> gbufferFormats, bool depth)
{
  const auto &mLogDeviceM = mContext->getLogicalDeviceManager();
  const auto &mPhysDeviceM = mContext->getPhysicalDeviceManager();
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

  const auto &mLogDeviceM = mContext->getLogicalDeviceManager();
  const auto &mPhysDeviceM = mContext->getPhysicalDeviceManager();
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

  const auto &mLogDeviceM = mContext->getLogicalDeviceManager();
  const auto &mPhysDeviceM = mContext->getPhysicalDeviceManager();
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
void Renderer::initRenderingRessources(Scene &scene, const AssetRegistry &registry)
{
  const VkDevice &device = mContext->getLogicalDeviceManager().getLogicalDevice();
  const VkPhysicalDevice &physDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
  const uint32_t indice = mContext->getPhysicalDeviceManager().getIndices().graphicsFamily.value();

  // NOT CONFIDENt
  // Descriptor from Scene
  // Create pipeline
 
  GpuResourceUploader uploader(*mContext, registry, mMaterialDescriptors, mPipelineM);

  for (auto &node : scene.nodes)
  {

    const auto &mesh = mRegistry->get(node.mesh);

    // resolve materials globally
    for (auto matId : mesh->materialIds)
    {
      PipelineLayoutConfig sceneConfig{{mFrameHandler.getCurrentFrameData().mDescriptor.getDescriptorLat(0)}, scene.sceneLayout.pushConstants};

      std::cout << "Material setup \n";

      const auto &mat = mRegistry->get(matId);

      auto layout = MaterialLayoutRegistry::Get(mat->mType);
      int matLayoutIdx = mMaterialDescriptors.getOrCreateSetLayout(device, layout.descriptorSetLayoutsBindings);
      sceneConfig.descriptorSetLayouts.push_back(mMaterialDescriptors.getDescriptorLat(matLayoutIdx));
      int pipelineEntryIndex = requestPipeline(sceneConfig, vertPath, fragPath);

      mGpuRegistry.add(matId, std::function<MaterialGPU()>([&]()
                                                           { return uploader.uploadMaterialGPU(matId, mGpuRegistry, matLayoutIdx, pipelineEntryIndex); }));
    }
  }
  // NOT CONFIDENt
  // TODO:
  // Important:
  // This can't stay longterm

  mRScene.syncFromScene(scene, registry, mGpuRegistry, uploader);
  std::cout << "Ressourcess uploaded" << std::endl;
  std::cout << "Scene Ressources Initialized" << std::endl;
};

void Renderer::deinitSceneRessources(Scene &scene)
{
  const auto allocator = mContext->getLogicalDeviceManager().getVmaAllocator();
  const auto device = mContext->getLogicalDeviceManager().getLogicalDevice();

  mRScene.destroy(mContext->getLogicalDeviceManager().getLogicalDevice(), allocator);

  // Below is more destroy Renderer than anything else
  mGBuffers.destroy(device, allocator);

  mGpuRegistry.clearAll(device, allocator);
  for (int i = 0; i < mContext->mSwapChainM.GetSwapChainImageViews().size(); i++)
  {
    auto &frameData = mFrameHandler.getCurrentFrameData();
    frameData.mFramebuffer.destroyFramebuffers(device);
    mFrameHandler.advanceFrame();
  }

  mFrameHandler.destroyFramesData(device);
  mMaterialDescriptors.destroyDescriptorLayout(device);
  mMaterialDescriptors.destroyDescriptorPool(device);

  mRenderPassM.destroyAll(device);
  mPipelineM.destroy(device);
}

int Renderer::requestPipeline(const PipelineLayoutConfig &config,
                              const std::string &vertexPath,
                              const std::string &fragmentPath)
{

  const VkDevice &device = mContext->getLogicalDeviceManager().getLogicalDevice();

  // For dynamic rendering
  // slice = std::vector<VkFormat>();
  // Slice asssuming  attachment last attachment is the depth

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

  renderQueue.build(mRScene);

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

//Handle non Material object
void Renderer::drawFrame(const SceneData &sceneData)
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

  for (const Drawable *draw : renderQueue.getDrawables())
  {
    // Todo: Sorting drawables to minimize binding
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipeline());

    auto *meshGpu = mGpuRegistry.get(draw->meshGPU);
    auto *materialGpu = mGpuRegistry.get(draw->materialGPU);

    // Bind descriptors
    std::vector<VkDescriptorSet> sets = {
        frameRess.mDescriptor.getSet(0),
        mMaterialDescriptors.getSet(materialGpu->descriptorIndex)};

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPipelineM.getPipelineLayout(), 0,
                            sets.size(), sets.data(),
                            0, nullptr);

    // Bind vertex
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &meshGpu->vertexBuffer, offsets);
    vkCmdBindIndexBuffer(cmd, meshGpu->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // Push constants
    vkCmdPushConstants(cmd, mPipelineM.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &sceneData.viewproj);

    // Draw
    vkCmdDrawIndexed(cmd, draw->indexCount, 1, draw->indexOffset, 0, 0);

    // Fake multi Viewport is basically this
    /*
    viewport.width = static_cast<float>(frameExtent.width);
    viewport.height = static_cast<float>(frameExtent.height/2);
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);
    */
  }
};