#pragma once
#include "TextureC.h"
#include "BaseVk.h" //Todo: move away
#include <glm/glm.hpp> //COLOR header ?

class RenderTargetInfo;
struct Material : AssetBase
{
  //static AssetType getStaticType() { return AssetType::Material; }
  //AssetType type() const override { return getStaticType(); }
  
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
