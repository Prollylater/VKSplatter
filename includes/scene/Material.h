#pragma once
#include "BaseVk.h"
#include "config/PipelineConfigs.h"
#include "ContextController.h"

class PipelineManager;
class Texture;
class RenderTargetInfo;

enum class MaterialType
{
  PBR,
  None
};
// Opaque, Transparent etc...

struct Material
{
  MaterialType mType = MaterialType::PBR;
  int pipelineEntryIndex = -1;
  int matLayoutIndex = -1;


  // Ressources
  Texture *albedoMap = nullptr;
  Texture *normalMap = nullptr;
  Texture *metallicMap = nullptr;
  Texture *roughnessMap = nullptr;
  // Texture *emissiveMap = nullptr;
  // Texture *aoMap = nullptr;
  //  Some Buffer for stuff ?

  struct MaterialConstants
  {
    // Base colors
    glm::vec4 albedoColor = glm::vec4(1.0f);
    glm::vec4 emissive = glm::vec4(0.0f);

    // PBR factors
    float metallic = 0.0f;  // 0 = dielectric, 1 = metal
    float roughness = 1.0f; // 0 = smooth, 1 = rough
    float padding[2];
  } mConstants;

  // Todo: Not quite needed
  PipelineLayoutDescriptor materialLayout;

  struct MaterialLayoutRegistry
  {
    static const PipelineLayoutDescriptor &Get(MaterialType type)
    {
      switch (type)
      {
      case MaterialType::None:

        static PipelineLayoutDescriptor UnlitLayout = []
        {
          PipelineLayoutDescriptor layout;
          layout.addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
          return layout;
        }();

        return UnlitLayout;
      case MaterialType::PBR:
      default:
        static PipelineLayoutDescriptor PBRLayout = []
        {
          PipelineLayoutDescriptor layout;
          layout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
          layout.addDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // albedo
          layout.addDescriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // normal
          layout.addDescriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // metal/rough
          layout.addDescriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // ao
          // layout.addDescriptor(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // emissive
          return layout;
        }();

        return PBRLayout;
      }
    }
  };

  void requestPipeline(VulkanContext &ctx, VertexFlags flags);
  void requestPipeline(VulkanContext &ctx, PipelineLayoutDescriptor materialLayout, std::string vertexShader, std::string fragmentShader, VertexFlags flags);
  void setType(MaterialType type)
  {
    mType = type;
  }
};
