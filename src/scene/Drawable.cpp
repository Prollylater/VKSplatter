#include "Drawable.h"


    //Binding 

    bool Drawable::bindInstanceBuffer(VkCommandBuffer cmd, const Drawable &drawable, uint32_t bindingIndex)
    {
        if (drawable.hotInstanceGPU.getID() != INVALID_ASSET_ID)
        {
            //VkDescriptorBufferInfo bufferInfo{};
            //bufferInfo.buffer = drawable.instanceGPU.instanceBuffer;
            //bufferInfo.offset = 0;
            //bufferInfo.range = drawable.instanceGPU.instanceCount * drawable.instanceGPU.instanceStride;        
            return true;
        }
        return false;

    }
