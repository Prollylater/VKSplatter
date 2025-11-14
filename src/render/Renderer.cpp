#include "Renderer.h"
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"
#include "Texture.h"
#include "Material.h"
#include "ContextController.h"

/*
Thing not implementend:
Cleaning Image/Depth Stencil/Manually clean attachements
*/

void Renderer::createFramesData(uint32_t framesInFlightCount)
{
  auto logicalDevice = mContext->getLogicalDeviceManager().getLogicalDevice();
  auto physicalDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
  auto graphicsFamilyIndex = mContext->getPhysicalDeviceManager().getIndices().graphicsFamily.value();

  mFrameHandler.createFramesData(logicalDevice, physicalDevice, graphicsFamilyIndex, framesInFlightCount);
}
static constexpr bool dynamic = true;

// TODO:
// Could be overloaded with  RenderTargetConfig that will mean dynamic
// Render Pass that will mean not dynamic render
void Renderer::initRenderInfrastructure()
{
  std::cout << "Init Render Infrastructure" << std::endl;

  const auto &mLogDeviceM = mContext->getLogicalDeviceManager();
  const auto &mPhysDeviceM = mContext->getPhysicalDeviceManager();
  const auto &mSwapChainM = mContext->getSwapChainManager();

  const VkDevice &device = mLogDeviceM.getLogicalDevice();
  const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();
  mGBuffers.init(mSwapChainM.getSwapChainExtent());

  const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();

  // Onward is just hardcoded stuff not meant to be dynamic yet
  const VmaAllocator &allocator = mLogDeviceM.getVmaAllocator();

  if (dynamic)
  {
    RenderTargetConfig defRenderPass;
    defRenderPass.addAttachment(mSwapChainM.getSwapChainImageFormat().format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
        .addAttachment(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

    // Create the actual offscreen G-buffers (excluding swapchain & depth)
    std::vector<VkFormat> gbufferFormats = defRenderPass.getAttachementsFormat();

    // Remove first (swapchain) and last (depth)
    if (gbufferFormats.size() > 2)
    {
      gbufferFormats = {gbufferFormats.begin() + 1, gbufferFormats.end() - 1};
    }
    else
    {
      gbufferFormats.clear();
    }

    mGBuffers.createGBuffers(mLogDeviceM, mPhysDeviceM, gbufferFormats, allocator);
    mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat, allocator);

    // 3Collect image views for dynamic rendering
    std::vector<VkImageView> colorViews;
    colorViews.reserve(mGBuffers.colorBufferNb());
    for (size_t index = 0; index < mGBuffers.colorBufferNb(); index++)
    {
      colorViews.push_back(mGBuffers.getColorImageView(index));
    }

    // Configure per-frame rendering info
    mFrameHandler.createFramesDynamicRenderingInfo(defRenderPass, colorViews, mGBuffers.getDepthImageView(), mSwapChainM.GetSwapChainImageViews(),
                                                   mSwapChainM.getSwapChainExtent());
  }
  else
  {
    // Defining the Render Pass Config as the config can have use in Pipeline Description
    RenderPassConfig defConfigRenderPass = RenderPassConfig::defaultForward(mSwapChainM.getSwapChainImageFormat().format, depthFormat);
    mRenderPassM.initConfiguration(defConfigRenderPass);

    // Get the proper format which could be done before Re
    // Todo: std::span usage could make sense here and there
    mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat, allocator);
    mRenderPassM.createRenderPass(device, defConfigRenderPass);

    mFrameHandler.completeFrameBuffers(device, {mGBuffers.getDepthImageView()}, mRenderPassM.getRenderPass(),
                                       mSwapChainM.GetSwapChainImageViews(),
                                       mSwapChainM.getSwapChainExtent());
  }
};

void Renderer::updateUniformBuffers(VkExtent2D swapChainExtent)
{
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto currentTime = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

  ubo.proj[1][1] *= -1;

  // Copy into persistently mapped buffer
  memcpy(mFrameHandler.getCurrentFrameData().mCameraMapping, &ubo, sizeof(ubo));
};

// All the rest or fram eressourees
void Renderer::initSceneRessources(Scene &scene)
{
  // Todo: Trim down this function

  std::cout << "Init Scene Ressources \n" ;
  const VkPhysicalDevice &physDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
  const QueueFamilyIndices &indicesFamily = mContext->getPhysicalDeviceManager().getIndices();
  const LogicalDeviceManager &deviceM = mContext->getLogicalDeviceManager();
  const VmaAllocator &allocator = mContext->getLogicalDeviceManager().getVmaAllocator();
  const VkDevice &device = mContext->getLogicalDeviceManager().getLogicalDevice();
  const uint32_t indice = indicesFamily.graphicsFamily.value();
  auto &swapChainM = mContext->getSwapChainManager();

  // Setup Pipeline
  mPipelineM.initialize(device, "");

  // Descriptor from Scene
  std::cout << "Init Various Descriptor \n" << indice<<std::endl;
  auto assetMesh = mRegistry->load<Mesh>(MODEL_PATH);
  const auto &materialIds = mRegistry->get(assetMesh)->materialIds;
  SceneNode node{assetMesh, materialIds[0]};
  scene.addNode(node);

  std::cout<<scene.nodes[0].material.id << " &" << scene.nodes[0].mesh.id <<std::endl; 
  // Init Frame Descriptor set of Scene
  std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutBindings = {scene.sceneLayout.descriptorSetLayouts};
  mFrameHandler.createFramesDescriptorSet(device, layoutBindings);
  mMaterialManager.createDescriptorPool(deviceM.getLogicalDevice(), 10, {});

  std::cout << "Handling Pipeline and Material \n";

  PipelineLayoutConfig sceneConfig{{mFrameHandler.getCurrentFrameData().mDescriptor.getDescriptorLat(0)}, scene.sceneLayout.pushConstants};
  for (auto &material : materialIds)
  {
    auto mat = mRegistry->get(material);
    auto text = mRegistry->get(mat->albedoMap);

    // The texture name is already know because it come from a registry
    text->createTextureImage(physDevice, deviceM,TEXTURE_PATH, indice, allocator);
    text->createTextureImageView(device);
    text->createTextureSampler(device, physDevice);

    // Material
    mat->requestPipelineCreateInfo();

    mat->matLayoutIndex = mMaterialManager.getOrCreateSetLayout(device,
                                                                mat->materialLayoutInfo.descriptorSetLayouts);
    mat->pipelineEntryIndex = requestPipeline(sceneConfig, mMaterialManager.getDescriptorLat(mat->matLayoutIndex), vertPath, fragPath);
  }

  // Upload GPU Data
  std::cout << "Upload Mesh & Material" << indice << std::endl;
  uploadScene(scene);
  // Effective Binding of Desciptor
  for (int i = 0; i < swapChainM.GetSwapChainImageViews().size(); i++)
  {
    auto &frameData = mFrameHandler.getCurrentFrameData();
    auto descriptorBuffer = frameData.mCameraBuffer.getDescriptor();

    std::cout << "Updating Frame Data Set" << std::endl;
    // Todo: Should be elsewhere
    //  Update descriptor sets with actual data
    std::vector<VkWriteDescriptorSet> writes = {
        vkUtils::Descriptor::makeWriteDescriptor(frameData.mDescriptor.getSet(0), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorBuffer)};
    frameData.mDescriptor.updateDescriptorSet(device, 0, writes);
    mFrameHandler.advanceFrame();
  }

  // Update/Store UBO
  // Todo: Need to introduce a proper camera
  updateUniformBuffers(swapChainM.getSwapChainExtent());

  std::cout << "Scene Ressources Initialized" << std::endl;
};

void Renderer::deinitSceneRessources(Scene &scene)
{
  const auto allocator = mContext->getLogicalDeviceManager().getVmaAllocator();
  const auto device = mContext->getLogicalDeviceManager().getLogicalDevice();

  mRScene.destroy(mContext->getLogicalDeviceManager().getLogicalDevice(), allocator);

  //Below is more destroy Renderer than anything else
  mGBuffers.destroy(device, allocator);

  for (int i = 0; i < mContext->mSwapChainM.GetSwapChainImageViews().size(); i++)
  {
    auto &frameData = mFrameHandler.getCurrentFrameData();
    frameData.mFramebuffer.destroyFramebuffers(device);
    mFrameHandler.advanceFrame();
  }
  mFrameHandler.destroyFramesData(device);

  mMaterialManager.destroyDescriptorLayout(device);
  mMaterialManager.destroyDescriptorPool(device);

  mRenderPassM.destroyRenderPass(device);
  mPipelineM.destroy(device);
}

void Renderer::uploadScene(const Scene &scene)
{

  const auto &deviceM = mContext->getLogicalDeviceManager();
  const auto &physDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
  //Todo const auto& thing
  const auto indice = mContext->getPhysicalDeviceManager().getIndices().graphicsFamily.value();
  const auto &allocator = mContext->getLogicalDeviceManager().getVmaAllocator();

  if(mContext->getPhysicalDeviceManager().getIndices().graphicsFamily.has_value()){
  }
  int index = 0;
  for (auto &node : scene.nodes)
  {
      std::cout<<node.material.id << " &" << node.mesh.id <<std::endl; 

    // Ideally we would have a count or traverse the nodes
    mRScene.drawables.resize(scene.nodes.size());
    auto &gpuNodes = mRScene.drawables[index];

    gpuNodes.createMeshGPU(*mRegistry->get(node.mesh), deviceM, physDevice, indice);
    gpuNodes.createMaterialGPU(*mRegistry, *mRegistry->get(node.material), deviceM, mMaterialManager, physDevice, indice);
  }
  std::cout << "Ressourcess uploaded" << std::endl;
}

int Renderer::requestPipeline(const PipelineLayoutConfig &config, VkDescriptorSetLayout materialLayout,
                              const std::string &vertexPath,
                              const std::string &fragmentPath)
{

  const VkDevice &device = mContext->getLogicalDeviceManager().getLogicalDevice();

  // For dynamic rendering
  // std::vector<VkFormat> slice;
  // We get SLice from the Gbuffer i guess
  // slice = std::vector<VkFormat>();
  // Slice asssuming  attachment last attachment is the depth

  PipelineBuilder builder;
  builder.setShaders({vertexPath, fragmentPath})
      .setInputConfig({.vertexFormat = VertexFormatRegistry::getStandardFormat()});

  if (dynamic)
  {
    auto attachments = mGBuffers.getAllFormats();
    attachments.insert(attachments.begin(), mContext->getSwapChainManager().getSwapChainImageFormat().format);
    std::vector<VkFormat> clrAttach(attachments.begin(), attachments.end() - 1);

    builder.setDynamicRenderPass(clrAttach, attachments.back());
  }
  else
  {
    builder.setRenderPass(mRenderPassM.getRenderPass());
  }

  // Todo: This is unecessary on the long term
  PipelineLayoutConfig layoutConfig = config;
  layoutConfig.descriptorSetLayouts.push_back(materialLayout);
  layoutConfig.pushConstants = config.pushConstants;

  builder.setUniform(layoutConfig);
  std::cout << "CreatePipeline \n";

  return mPipelineM.createPipelineWithBuilder(device, builder);
};

// Drawing Loop Functions

// Semaphore and command buffer tied to frames
void Renderer::drawFrame(bool framebufferResized, GLFWwindow *window)
{
  // Handle fetching
  VkDevice device = mContext->mLogDeviceM.getLogicalDevice();
  SwapChainManager &chain = mContext->getSwapChainManager();

  // Todo: Not sure about exposing this as non const
  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();

  // In a real app we could do other stuff while waiting on Fence
  // Or maybe multiple submitted task with each their own fence they get so that they can move unto a specific task
  frameRess.mSyncObjects.waitFenceSignal(device);

  uint32_t imageIndex;

  bool resultAcq = chain.aquireNextImage(device,
                                         frameRess.mSyncObjects.getImageAvailableSemaphore(),
                                         imageIndex);

  if (!resultAcq)
  {
    // Todo: Should be in swapchain
    // recreateSwapChain(device, window);
    // return;
  }

  frameRess.mSyncObjects.resetFence(device);

  // Update Camera
  updateUniformBuffers(mContext->mSwapChainM.getSwapChainExtent());

  //
  renderQueue.build(mRScene);
  // recordCommandBuffer(imageIndex);
  recordCommandBufferD(imageIndex);

  // Submit Info set up
  mContext->mLogDeviceM.submitFrameToGQueue(
      frameRess.mCommandPool.get(),
      frameRess.mSyncObjects.getImageAvailableSemaphore(),
      frameRess.mSyncObjects.getRenderFinishedSemaphore(),
      frameRess.mSyncObjects.getInFlightFence());
  VkResult result = mContext->mLogDeviceM.presentImage(chain.GetChain(),
                                                       frameRess.mSyncObjects.getRenderFinishedSemaphore(),
                                                       imageIndex);

  // Recreate the Swap Chain if suboptimal

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
}

void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
  // Handle fetching
  // Todo: Not sure about exposing this
  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();
  auto &commandPoolM = frameRess.mCommandPool;

  VkExtent2D frameExtent = mContext->mSwapChainM.getSwapChainExtent();
  const VkCommandBuffer command = commandPoolM.get();

  commandPoolM.beginRecord();
  mRenderPassM.startPass(command, frameRess.mFramebuffer.GetFramebuffers().at(0), frameExtent);

  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipeline());

  // Command
  // Function in pipeline that check the size ?
  // Todo: THink about where to put this

  // Add this for dynamic Pipelin
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(frameExtent.width);
  viewport.height = static_cast<float>(frameExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = frameExtent;
  vkCmdSetScissor(command, 0, 1, &scissor);

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
    bind material resources  // sets 2,3
  }
}
}
  */

  // 0 is not quite right
  // I m also not quite sure how location is determined, just through the attributes description
  // 1 interleaved buffer here

  // MESH DRAWING

  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipeline());

  VkDeviceSize offsets[] = {0};

  for (const Drawable *draw : renderQueue.getDrawables())
  {
    std::vector<VkDescriptorSet> sets = {frameRess.mDescriptor.getSet(0),
                                         mMaterialManager.getSet(draw->materialGPU.descriptorIndex)};
    // Could also usetwo call
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPipelineM.getPipelineLayout(), 0, sets.size(),
                            sets.data(), 0,
                            nullptr);
    vkCmdBindVertexBuffers(command, 0, 1, &draw->meshGPU.vertexBuffer, offsets);
    vkCmdBindIndexBuffer(command, draw->meshGPU.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(command, mPipelineM.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(UniformBufferObject), frameRess.mCameraMapping);
    vkCmdDrawIndexed(command, draw->meshGPU.indexCount, 1, 0, 0, 0);
  }
  // Multi Viewport is basically this
  /*
  viewport.width = static_cast<float>(frameExtent.width);
  viewport.height = static_cast<float>(frameExtent.height/2);
  vkCmdSetViewport(command, 0, 1, &viewport);
  vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);
  */

  mRenderPassM.endPass(command);
  commandPoolM.endRecord();
}

// Create Transition based on renderpassinfo when possible
void Renderer::recordCommandBufferD(uint32_t imageIndex)
{
  FrameResources &frameRess = mFrameHandler.getCurrentFrameData();
  auto &commandPoolM = frameRess.mCommandPool;

  VkExtent2D frameExtent = mContext->mSwapChainM.getSwapChainExtent();
  const VkCommandBuffer command = commandPoolM.get();
  // Todo: Untie Frame Iamge and Other ?
  const auto &images = mContext->mSwapChainM.GetSwapChainImages();

  commandPoolM.beginRecord();

  // Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL for rendering
  auto transObj = vkUtils::Texture::makeTransition(images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
  transObj.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  transObj.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  transObj.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  vkUtils::Texture::recordImageMemoryBarrier(command, transObj);
  //  Transition

  vkCmdBeginRendering(command, &frameRess.mDynamicPassInfo.info);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(frameExtent.width);
  viewport.height = static_cast<float>(frameExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = frameExtent;
  vkCmdSetScissor(command, 0, 1, &scissor);

  // MESH DRAWING

  vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipeline());

  VkDeviceSize offsets[] = {0};

  for (const Drawable *draw : renderQueue.getDrawables())
  {
    std::vector<VkDescriptorSet> sets = {frameRess.mDescriptor.getSet(0),
                                         mMaterialManager.getSet(draw->materialGPU.descriptorIndex)};

    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPipelineM.getPipelineLayout(), 0, sets.size(),
                            sets.data(), 0,
                            nullptr);
    vkCmdBindVertexBuffers(command, 0, 1, &draw->meshGPU.vertexBuffer, offsets);
    vkCmdBindIndexBuffer(command, draw->meshGPU.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(command, mPipelineM.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(UniformBufferObject), frameRess.mCameraMapping);
    vkCmdDrawIndexed(command, draw->meshGPU.indexCount, 1, 0, 0, 0);
  }
  vkCmdEndRendering(command);

  // From here end the old ccode not using Render PAss
  //  Transition
  auto transObjb = vkUtils::Texture::makeTransition(images[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);
  transObjb.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  transObjb.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  transObj.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  transObj.dstAccessMask = 0;

  vkUtils::Texture::recordImageMemoryBarrier(command, transObjb);

  commandPoolM.endRecord();
}
