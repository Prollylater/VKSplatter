#pragma once
#include "BaseVk.h"
#include "config/PipelineConfigs.h"
#include "ContextController.h"
#include "TextureC.h"
//class TextureCPU;
class RenderTargetInfo;

// Opaque, Transparent etc...

struct Material : AssetBase
{
  MaterialType mType = MaterialType::PBR;

  AssetID<TextureCPU> albedoMap;
  AssetID<TextureCPU> normalMap;
  AssetID<TextureCPU> metallicMap;
  AssetID<TextureCPU> roughnessMap;

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

  void setType(MaterialType type)
  {
    mType = type;
  }
};
