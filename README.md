### Vulkan Shared Memory

***

I made this Project to show how I use the VK_KHR_external_memory extension for sharing memory between two vulkan applications. In the future I hope to add examples for Vulkan - OpenGL and vulkan - DX11/DX12, but that might take a while.

### Building

***


## Currently does not build!

To compile the project run these commands in the base of the project directory.

> mkdir build && cd build
> conan install .. --build=missing
> cmake .. --preset conan-release
> cd Release
> make

