cmake_minimum_required(VERSION 3.10)
project(VulkanGLFWExample)

#Todo: Better handling of the CMake ?
# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Find Vulkan and GLFW
find_package(Vulkan REQUIRED)

# GLFW setup
find_package(glfw3 REQUIRED)

#Directories
set(SOURCE_DIR engine/src)
set(USER_DIR application)
set(INCLUDE_DIR includes)
set(THIRD_PARTY thirdparty)


if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")


# Source files
set(SOURCES
    ${USER_DIR}/main.cpp
    ${USER_DIR}/HelloTriangle.cpp  

    ${SOURCE_DIR}/assetsystem/AssetIO.cpp   
    ${SOURCE_DIR}/assetsystem/AssetRegistry.cpp   
    ${SOURCE_DIR}/assetsystem/ResourceSystem.cpp 
    ${SOURCE_DIR}/assetsystem/Material.cpp   
    ${SOURCE_DIR}/assetsystem/Mesh.cpp   
    
    ${SOURCE_DIR}/core/filesystem/Filesystem.cpp   
    ${SOURCE_DIR}/core/logging/Logger.cpp   
        
    ${SOURCE_DIR}/scene/Camera.cpp   
    ${SOURCE_DIR}/scene/Drawable.cpp   
    ${SOURCE_DIR}/scene/Scene.cpp   
    ${SOURCE_DIR}/scene/Light.cpp   
    #${SOURCE_DIR}/scene/geometry/Frustrum.cpp   
    #${SOURCE_DIR}/scene/geometry/Extents.cpp   
    ${SOURCE_DIR}/scene/Transform.cpp   
    
    ${SOURCE_DIR}/vulkan/Buffer.cpp   
    ${SOURCE_DIR}/vulkan/CommandPool.cpp   
    ${SOURCE_DIR}/vulkan/LogicalDevice.cpp   
    ${SOURCE_DIR}/vulkan/PhysicalDevice.cpp   
    ${SOURCE_DIR}/vulkan/Pipeline.cpp   
    ${SOURCE_DIR}/vulkan/ContextController.cpp   
    ${SOURCE_DIR}/vulkan/Texture.cpp   
    ${SOURCE_DIR}/vulkan/GBuffers.cpp   
    ${SOURCE_DIR}/vulkan/RenderPass.cpp   
    ${SOURCE_DIR}/vulkan/SwapChain.cpp   
    ${SOURCE_DIR}/vulkan/SyncObjects.cpp   
    ${SOURCE_DIR}/vulkan/QueueFam.cpp   
    ${SOURCE_DIR}/vulkan/Descriptor.cpp   
    ${SOURCE_DIR}/vulkan/VulkanInstance.cpp

    ${SOURCE_DIR}/render/FrameHandler.cpp  
    ${SOURCE_DIR}/render/GPUResourceSystem.cpp   
    ${SOURCE_DIR}/render/GPUResource.cpp   
    ${SOURCE_DIR}/render/VertexDescriptions.cpp   
    ${SOURCE_DIR}/render/MaterialSystem.cpp   
    ${SOURCE_DIR}/render/Renderer.cpp  
    ${SOURCE_DIR}/render/RenderScene.cpp  
    ${SOURCE_DIR}/render/Passes.cpp  


    ${SOURCE_DIR}/vulkan/utils/RessourceHelper.cpp   
    ${SOURCE_DIR}/vulkan/utils/PipelineHelper.cpp   

    ${SOURCE_DIR}/core/WindowVk.cpp
    ${SOURCE_DIR}/core/Application.cpp
    ${SOURCE_DIR}/core/Clock.cpp
    
    ${THIRD_PARTY}/stb_image_loader.cpp
    ${THIRD_PARTY}/vkmaUsage.cpp
)


file(COPY ressources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


# Main executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
# This go a bit too deep in the directory to include
target_include_directories(VulkanGLFWExample PRIVATE
${Vulkan_INCLUDE_DIRS} 
${GLFW_INCLUDE_DIRS}
${THIRD_PARTY}
${USER_DIR}
${SOURCE_DIR}/vulkan
${SOURCE_DIR}/assetsystem
${SOURCE_DIR}/core
${SOURCE_DIR}/scene
${SOURCE_DIR}/render
${SOURCE_DIR})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
${Vulkan_LIBRARIES} glfw)

##############################################
########################Testing
##############################################


find_package(Catch2 3 REQUIRED)
# These tests can use the Catch2-provided main
add_executable(allocator_tests ${SOURCE_DIR}/core/memory/tests/Memorytest.cpp ${SOURCE_DIR}/core/memory/Memory.cpp)
add_executable(container_tests ${SOURCE_DIR}/core/containers/tests/Containerstest.cpp ${SOURCE_DIR}/core/memory/Memory.cpp)

set_target_properties(
allocator_tests 
container_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/tests )


add_library(test_common INTERFACE)
target_include_directories(test_common INTERFACE
    ${SOURCE_DIR}/core/memory ${SOURCE_DIR}/core/containers
)
target_link_libraries(test_common INTERFACE
    Catch2::Catch2WithMain 
)
target_link_libraries(allocator_tests PRIVATE test_common)
target_link_libraries(container_tests PRIVATE test_common)

# These tests need their own main
#add_executable(custom-main-tests test.cpp test-main.cpp)
#target_link_libraries(custom-main-tests PRIVATE Catch2::Catch2)

