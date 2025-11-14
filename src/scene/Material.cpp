
#include "Material.h"

void Material::requestPipelineCreateInfo()
{
  materialLayoutInfo = MaterialLayoutRegistry::Get(mType);
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
}

