
#include "Material.h"
#include "Pipeline.h"

void Material::requestPipeline(VulkanContext &ctx, VertexFlags flags)
{
  materialLayout = MaterialLayoutRegistry::Get(mType);
  std::string vertexShader = vertPath;
  std::string fragmentShader = fragPath;

  switch (mType)
  {
  case MaterialType::PBR:
  default:
    vertexShader = vertPath;
    fragmentShader = fragPath;
    break;
  }

  // Device presence ?
  // Some has should prevent SetLayout to be repeated
  requestPipeline(ctx, materialLayout, vertexShader, fragmentShader, flags);
}

void Material::requestPipeline(VulkanContext &ctx, PipelineLayoutDescriptor materialLayout, std::string vertexShader, std::string fragmentShader, VertexFlags flags)
{
  matLayoutIndex = ctx.mMaterialManager.getOrCreateSetLayout(ctx.getLogicalDeviceManager().getLogicalDevice(),
                                                 materialLayout.descriptorSetLayouts);
  pipelineEntryIndex = ctx.requestPipeline(ctx.mMaterialManager.getDescriptorLat(matLayoutIndex), vertexShader, fragmentShader, flags);
}
