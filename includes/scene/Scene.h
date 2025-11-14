#pragma once
#include "BaseVk.h"
#include "Mesh.h"
#include "Drawable.h"
#include "Material.h"
#include "RessourcesGPU.h"


class Scene
{
public:
    Scene(){
        //Todo: Temporary position
        sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject));
    };
    ~Scene() = default;

    //Todo: Something about rebuild when an Asset become invalid
    std::vector<SceneNode> nodes;
    void addNode(SceneNode node){
        nodes.push_back(node);
    }
    void clearScene(){
        nodes.clear();
    }
        
  

    // Camera object
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;

    // Light
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
    
    PipelineSetLayoutBuilder sceneLayout;

};



class RenderScene
{
public:
    RenderScene() = default;
    ~RenderScene() = default;

   //std::vector<Entity> entities;
    void destroy(VkDevice device, VmaAllocator alloc)
    {
        for (auto &drawable : drawables)
        {
            drawable.materialGPU.destroy(device, alloc);
            drawable.meshGPU.destroy(device, alloc);
        };
    };


    std::vector<Drawable> drawables;
    void syncFromScene(const Scene & scene);

    /*
    if (scene.hasChanged()) {
    renderScene.rebuild(scene, assetRegistry, gpuFactory);
    */
};


/*
class Scene
{
  
  With SceneGraph Here
};

class RenderScene
{
  Drawable here
};

*/