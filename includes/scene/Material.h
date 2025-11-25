#pragma once
#include "BaseVk.h"
#include "config/PipelineConfigs.h"
#include "ContextController.h"

class Texture;
class RenderTargetInfo;

// Opaque, Transparent etc...

struct Material
{
  MaterialType mType = MaterialType::PBR;

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

  void setType(MaterialType type)
  {
    mType = type;
  }
};
