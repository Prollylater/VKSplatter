#pragma once
#include "BaseVk.h"
#include "config/PipelineConfigs.h"
#include "ContextController.h"

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

  //Todo:
  //Those value are potentially problematic,
  //Must be reset if a pipeline is destroeyd or a matLayout especially since those are GPU ressources
  //Or someone should know the Material to ring it if needed
  //They are also yet another reason we use asset registry as non cosnt
  int pipelineEntryIndex = -1;
  int matLayoutIndex = -1;

   // This could serve as a way 
  PipelineSetLayoutBuilder materialLayoutInfo;


  // Ressources
  AssetID<Texture> albedoMap;
  AssetID<Texture> normalMap;
  AssetID<Texture> metallicMap;
  AssetID<Texture> roughnessMap;

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

 
  struct MaterialLayoutRegistry
  {
    static const PipelineSetLayoutBuilder &Get(MaterialType type)
    {
      switch (type)
      {
      case MaterialType::None:

        static PipelineSetLayoutBuilder UnlitLayout = []
        {
          PipelineSetLayoutBuilder layout;
          layout.addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
          return layout;
        }();

        return UnlitLayout;
      case MaterialType::PBR:
      default:
        static PipelineSetLayoutBuilder PBRLayout = []
        {
          PipelineSetLayoutBuilder layout;
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

  //Todo: Not very useful
  void requestPipelineCreateInfo();
  void setType(MaterialType type)
  {
    mType = type;
  }
};
