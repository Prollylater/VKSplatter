#include "Drawable.h"
/*
   void Drawable::createMeshGPU(const Mesh &mesh, const LogicalDeviceManager &deviceM, VkPhysicalDevice physDevice, const uint32_t indice, bool SSBO = false)
    {
        meshGPU = MeshGPU::createMeshGPU(mesh, deviceM, physDevice, indice, SSBO);
    };

    // Todo: If registry is passed might as well just pass an handle
    void Drawable::createMaterialGPU(AssetRegistry &registry, const Material &material, const LogicalDeviceManager &deviceM, DescriptorManager &descriptor, VkPhysicalDevice physDevice, const uint32_t indice)
    {
        materialGPU = MaterialGPU::createMaterialGPU(registry, material, deviceM, descriptor, physDevice, indice);
    };

    void Drawable::createInstanceGPU(const std::vector<InstanceData>& instances, const LogicalDeviceManager &deviceM, VkPhysicalDevice physDevice, const uint32_t indice)
    {
        instanceGPU = InstanceGPU::createInstanceGPU(instances, deviceM, physDevice, indice);
    };
*/

    //Binding 

    bool Drawable::bindInstanceBuffer(VkCommandBuffer cmd, const Drawable &drawable, uint32_t bindingIndex)
    {
        if (drawable.instanceGPU.instanceBuffer != VK_NULL_HANDLE)
        {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = drawable.instanceGPU.instanceBuffer;
            bufferInfo.offset = 0;
            bufferInfo.range = drawable.instanceGPU.instanceCount * drawable.instanceGPU.instanceStride;        
            return true;
        }
        return false;

    }
