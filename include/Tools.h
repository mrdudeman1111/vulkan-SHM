#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

class Instance;
class PhysicalDevice;
class Device;
class Swapchain;
class RenderPass;
class DescriptorPool;
class Pipeline;
class CommandPool;

class Instance
{
public:
  VkInstance vulkanInstance;

  std::vector<const char*> Layers;
  std::vector<const char*> Extensions;

  GLFWwindow* Window;
  VkSurfaceKHR Surface;

  void Create(uint32_t inWidth, uint32_t inHeight, bool InitWindow);
private:
  void CreateWindow(uint32_t inWidth, uint32_t inHeight);
};

class PhysicalDevice
{
public:
  VkPhysicalDevice vulkanPhysicalDevice;
  uint32_t Local; // Device local memory index
  uint32_t Host; // Host Visibile Memory index

  uint32_t GrapicsFamily;

  void Create(Instance& Inst);

private:
  void QueryMemory();
};

class Device
{
public:
  VkDevice vulkanDevice;

  std::vector<const char*> Layers;
  std::vector<const char*> Extensions;

  VkQueue GraphicsQueue;

  void Create(PhysicalDevice& PhysDev);
  VkSemaphore GetSemaphore();
  VkFence GetFence();
  VkBuffer GetBuffer(PhysicalDevice& PDev, VkDeviceMemory& outMemory, uint32_t Size, VkBufferUsageFlags Usage);
};

class Swapchain
{
public:
  VkSwapchainKHR vulkanSwapchain;

  std::vector<VkImage> SwapchainImages;
  std::vector<VkImageView> SwapchainImageViews;
  std::vector<VkFramebuffer> FrameBuffers;

  VkSurfaceFormatKHR SwapchainFormat;

  void Create(Instance& Inst, PhysicalDevice& PDev, Device& Dev, VkExtent2D& Extent);
  void CreateFrameBuffers(RenderPass& pRP, Device& Dev, VkExtent2D& Extent);
};

class RenderPass
{
public:
  VkRenderPass vulkanRenderPass;

  VkExtent2D PassExtent;

  VkSubpassDescription Subpass;
  VkSubpassDescription TransitionSubpass;

  void Create(Swapchain& Swap, Device& Dev, VkExtent2D& Extent);
  void Begin(VkFramebuffer& RenderTarget, VkCommandBuffer& cmdBuff);
  void End(VkCommandBuffer& cmdBuff);
};

class DescriptorPool
{
  friend Pipeline;
public:
  VkDescriptorPool vulkanDescriptorPool;

  void CreatePool(Device& Dev, uint32_t DescriptorCount);
  void PushLayout(VkDescriptorSetLayout Layout, VkDescriptorPoolSize Size);

  VkDescriptorSet GetDescriptor(Device& Dev, uint32_t LayoutIndex);

private:
  std::vector<VkDescriptorSetLayout> Layouts;
  std::vector<VkDescriptorPoolSize> Sizes;
};

class Pipeline
{
public:
  VkPipeline vulkanPipeline;
  VkPipelineLayout PipelineLayout;

  void Create(RenderPass& pRP, Device& Dev, DescriptorPool& Pool, VkExtent2D& Extent);
  void Bind(VkCommandBuffer& cmdBuff);

private:
  VkShaderModule VertexModule;
  VkShaderModule FragmentModule;
};

class CommandPool
{
public:
  VkCommandPool vulkanCommandPool;

  void Create(PhysicalDevice& PDev, Device& Dev);
  VkCommandBuffer GetCommandBuffer(Device& Dev);
};

