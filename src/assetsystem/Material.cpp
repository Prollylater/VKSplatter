
#include "Material.h"

 void Material::setType(MaterialType type)
  {
    mType = type;
  }



  uint32_t Material::computeFlags() const
  {
    uint32_t flags = 0;

    if (albedoMap.isValid())
    {
      flags |= MAT_HAS_ALBEDO;
    }
    if (normalMap.isValid())
    {
      flags |= MAT_HAS_NORMAL;
    }
    if (metallicMap.isValid())
    {
      flags |= MAT_HAS_METALLIC;
    }
    if (roughnessMap.isValid())
    {
      flags |= MAT_HAS_ROUGHNESS;
    }
    if (emissiveMap.isValid())
    {
      flags |= MAT_HAS_EMISSIVE;
    }

    return flags;
  }