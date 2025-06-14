cmake_minimum_required(VERSION 3.10)
project(VulkanGLFWExample)

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
    ${SOURCE_DIR}/Buffer.cpp   
    ${SOURCE_DIR}/CommandPool.cpp   
    ${SOURCE_DIR}/LogicalDevice.cpp   
    ${SOURCE_DIR}/PhysicalDevice.cpp   
    ${SOURCE_DIR}/Pipeline.cpp   

    ${SOURCE_DIR}/Texture.cpp   

    ${SOURCE_DIR}/RenderPass.cpp   
    ${SOURCE_DIR}/SwapChain.cpp   
    ${SOURCE_DIR}/SyncObjects.cpp   
    ${SOURCE_DIR}/QueueFam.cpp   
    ${SOURCE_DIR}/Uniforms.cpp   
    ${SOURCE_DIR}/VertexDescriptions.cpp   

    ${SOURCE_DIR}/VulkanInstance.cpp
    ${SOURCE_DIR}/Texture.cpp
    ${THIRD_PARTY}/stb_image_loader.cpp

)

file(COPY ressources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(VulkanGLFWExample PRIVATE
${Vulkan_INCLUDE_DIRS} 
${GLFW_INCLUDE_DIRS}
${INCLUDE_DIR}
${THIRD_PARTY})

# Link libraries
#set(LINK_LIBS${Vulkan_LIBRARIES}     glfw)


# Link libraries
target_link_libraries(${PROJECT_NAME} 
${Vulkan_LIBRARIES} glfw)



