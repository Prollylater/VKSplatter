cmake_minimum_required(VERSION 3.10)
project(VulkanGLFWExample)
#Todo: Better handling of the CMake ?
# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Find Vulkan and GLFW
find_package(Vulkan REQUIRED)

# GLFW setup
find_package(glfw3 REQUIRED)


set(SOURCE_DIR src)
set(INCLUDE_DIR includes)
set(THIRD_PARTY thirdparty)

# Source files
set(SOURCES
    ${SOURCE_DIR}/main.cpp
    ${SOURCE_DIR}/HelloTriangle.cpp  

    ${SOURCE_DIR}/assetsystem/AssetIO.cpp   
    ${SOURCE_DIR}/assetsystem/AssetRegistry.cpp   
    ${SOURCE_DIR}/assetsystem/ResourceSystem.cpp 
    
    ${SOURCE_DIR}/scene/Camera.cpp   
    ${SOURCE_DIR}/scene/Drawable.cpp   
    ${SOURCE_DIR}/scene/Material.cpp   
    ${SOURCE_DIR}/scene/Mesh.cpp   
    ${SOURCE_DIR}/scene/Scene.cpp   


    ${SOURCE_DIR}/vulkan/Buffer.cpp   
    ${SOURCE_DIR}/vulkan/CommandPool.cpp   
    ${SOURCE_DIR}/vulkan/LogicalDevice.cpp   
    ${SOURCE_DIR}/vulkan/PhysicalDevice.cpp   
    ${SOURCE_DIR}/vulkan/Pipeline.cpp   
    ${SOURCE_DIR}/vulkan/ContextController.cpp   
    ${SOURCE_DIR}/vulkan/Texture.cpp   
    ${SOURCE_DIR}/vulkan/RenderPass.cpp   
    ${SOURCE_DIR}/vulkan/SwapChain.cpp   
    ${SOURCE_DIR}/vulkan/SyncObjects.cpp   
    ${SOURCE_DIR}/vulkan/QueueFam.cpp   
    ${SOURCE_DIR}/vulkan/Descriptor.cpp   
    ${SOURCE_DIR}/vulkan/VulkanInstance.cpp

    ${SOURCE_DIR}/render/RenderQueue.cpp  
    ${SOURCE_DIR}/render/FrameHandler.cpp   
    ${SOURCE_DIR}/render/RessourcesGPU.cpp   
    ${SOURCE_DIR}/render/VertexDescriptions.cpp   
    ${SOURCE_DIR}/render/Renderer.cpp   

    ${SOURCE_DIR}/vulkan/utils/RessourceHelper.cpp   
    ${SOURCE_DIR}/vulkan/utils/PipelineHelper.cpp   

    ${SOURCE_DIR}/core/WindowVk.cpp
    
    ${THIRD_PARTY}/stb_image_loader.cpp
    ${THIRD_PARTY}/vkmaUsage.cpp
)


file(COPY ressources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(VulkanGLFWExample PRIVATE
${Vulkan_INCLUDE_DIRS} 
${GLFW_INCLUDE_DIRS}
${INCLUDE_DIR}
${INCLUDE_DIR}/vulkan
${INCLUDE_DIR}/render
${INCLUDE_DIR}/scene
${INCLUDE_DIR}/assetsystem
${INCLUDE_DIR}/core
${THIRD_PARTY})

# Link libraries
#set(LINK_LIBS${Vulkan_LIBRARIES} glfw)


# Link libraries
target_link_libraries(${PROJECT_NAME} 
${Vulkan_LIBRARIES} glfw)



