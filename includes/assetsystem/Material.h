#pragma once

//Hold MaterialType
#include "BaseVk.h" //Todo: move away


#include "TextureC.h"
#include <glm/glm.hpp> //COLOR header ?

//Todo: 
//Add default material
//Change codeStyle to code_style
class RenderTargetInfo;
struct Material : AssetBase
{
  enum class LightingPath : uint8_t { Deferred, Forward, Both };
LightingPath mLightingPath = LightingPath::Forward;

  // static AssetType getStaticType() { return AssetType::Material; }
  // AssetType type() const override { return getStaticType(); }

  MaterialType mType = MaterialType::PBR;

  AssetID<TextureCPU> albedoMap;
  AssetID<TextureCPU> normalMap;
  AssetID<TextureCPU> metallicMap;
  AssetID<TextureCPU> roughnessMap;
  AssetID<TextureCPU> emissiveMap;

  struct MaterialConstants
  {
    //KS,KD,
    glm::vec4 albedoColor = glm::vec4(1.0f);
    glm::vec4 emissive = glm::vec4(0.0f);
    float metallic = 0.0f;  
    float roughness = 0.0f; 
    float specular = 0.0f;
    uint32_t flags;

    //Specularityt
  } mConstants;

  void setType(MaterialType type);
  enum MaterialFlags : uint32_t
  {
    MAT_HAS_ALBEDO = 1 << 0,
    MAT_HAS_NORMAL = 1 << 1,
    MAT_HAS_METALLIC = 1 << 2,
    MAT_HAS_ROUGHNESS = 1 << 3,
    MAT_HAS_EMISSIVE = 1 << 4,
  };

  uint32_t computeFlags() const;
};
