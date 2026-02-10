# VKSplatter ??

This project was started as both of a reason to practice Vulkan and some novel view rendering like Gaussian Splatting hence the name.
Currently it is very WIP, due to some conflicting vision view. 
Going forward, what is basically still, an ugly .exe will move toward a RHI/Framework at the bottom layer, a RenderEngine both as library.

# Current States 
* Forward rendering
  * Basic Phong Shader (Point/Directional Light)
  * Todo: Shadow Cascade
* Material abstraction (PBR-style layout)
* Descriptor layout registry
* Pipeline request system
* Mesh instancing support
* Vulkan Dynamic and legacy renderpass separation
* Resource lifetime tracking system/Memory Allocator...

# Not quite handled
* Better configuration options on the engine from "user-Layer"  
* Proper RenderGraph abstraction
* Unified dynamic/legacy render backend
* Pass dependency graph
* UI wise : Simple Glfw Window

# Build

* Build is pretty standard Cmake stuff, however at the current state too many change are still pending 
