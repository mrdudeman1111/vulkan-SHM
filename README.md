### Vulkan Shared Memory

***

 I was 17 when I started this project, and I made it to show how I use the VK_KHR_external_memory extension for sharing memory between two graphics applications. I had primarily written this project to remember what I had learned while writing W1reless and W1refree. In the future I had hoped to add examples for Vulkan - OpenGL and vulkan - DX11/DX12, but I have been busy with other projects.

### Building

***


## Currently does not build!

To compile the project run these commands in the base of the project directory.

> mkdir build && cd build
> 
> conan install .. --build=missing
> 
> cmake .. --preset conan-release
> 
> cd Release
> 
> make

