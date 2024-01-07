#include "Tools.h"

#include "Context.h"
#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <assert.h>
#include <iostream>
#include <string>

#include <vulkan/vulkan_core.h>

void Instance::CreateWindow(uint32_t inWidth, uint32_t inHeight)
{
  glfwInit();

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  Window = glfwCreateWindow(inWidth, inHeight, "ExternalImageRenderer", nullptr, nullptr);

  uint32_t glfwExtCount;
  const char** glfwExt = glfwGetRequiredInstanceExtensions(&glfwExtCount);

  for(uint32_t i = 0; i < glfwExtCount; i++)
  {
    Extensions.push_back(glfwExt[i]);
  }
}

void Instance::Create(uint32_t inWidth, uint32_t inHeight, bool InitWindow)
{
  VkResult Err;

  if(InitWindow) CreateWindow(inWidth, inHeight);

  VkInstanceCreateInfo InstanceCI{};
  InstanceCI.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  InstanceCI.enabledLayerCount = Layers.size();
  InstanceCI.ppEnabledLayerNames = (Layers.size() == 0) ? nullptr : Layers.data();
  InstanceCI.enabledExtensionCount = Extensions.size();
  InstanceCI.ppEnabledExtensionNames = (Extensions.size() == 0) ? nullptr : Extensions.data();

  if((Err = vkCreateInstance(&InstanceCI, nullptr, &vulkanInstance)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create instance with error: " + std::to_string(Err));
  }

  if(InitWindow) glfwCreateWindowSurface(vulkanInstance, Window, nullptr, &Surface);
}

void PhysicalDevice::QueryMemory()
{
  // Get Memory types
  {
    VkPhysicalDeviceMemoryProperties MemProps;
    vkGetPhysicalDeviceMemoryProperties(vulkanPhysicalDevice, &MemProps);

    Local = -1;
    Host = -1;

    for(uint32_t i = 0; i < MemProps.memoryTypeCount; i++)
    {
      if(MemProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && Local == -1)
      {
        Local = i;
      }
      else if(MemProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && Host == -1)
      {
        Host = i;
      }
    }

    assert(Local != -1 && Host != -1);
  }

  // Get queue families
  {
    uint32_t FamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &FamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> FamilyPoperties(FamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &FamilyCount, FamilyPoperties.data());

    GrapicsFamily = -1;

    for(uint32_t i = 0; i < FamilyCount; i++)
    {
      if(FamilyPoperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        GrapicsFamily = i;
      }
    }

    assert(GrapicsFamily != -1);
  }
}

void PhysicalDevice::Create(Instance& Inst)
{
  uint32_t pDevCount;
  vkEnumeratePhysicalDevices(Inst.vulkanInstance, &pDevCount, nullptr);
  std::vector<VkPhysicalDevice> PhysicalDevices(pDevCount);
  vkEnumeratePhysicalDevices(Inst.vulkanInstance, &pDevCount, PhysicalDevices.data());

  for(uint32_t i = 0; i < pDevCount; i++)
  {
    VkPhysicalDeviceProperties DeviceProps;
    vkGetPhysicalDeviceProperties(PhysicalDevices[i], &DeviceProps);

    if(DeviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
      vulkanPhysicalDevice = PhysicalDevices[i];
    }
  }

  if(vulkanPhysicalDevice == VK_NULL_HANDLE)
  {
    vulkanPhysicalDevice = PhysicalDevices[0];
  }

  assert(vulkanPhysicalDevice != VK_NULL_HANDLE);

  QueryMemory();
}

void Device::Create(PhysicalDevice& PhysDev)
{
  VkResult Err;

  float Prio[] = {1.f};

  VkDeviceQueueCreateInfo GraphicsQueueCI{};
  GraphicsQueueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  GraphicsQueueCI.queueCount = 1;
  GraphicsQueueCI.queueFamilyIndex = PhysDev.GrapicsFamily;
  GraphicsQueueCI.pQueuePriorities = Prio;

  VkDeviceCreateInfo DeviceCI{};
  DeviceCI.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  DeviceCI.queueCreateInfoCount = 1;
  DeviceCI.pQueueCreateInfos = &GraphicsQueueCI;
  DeviceCI.enabledExtensionCount = Extensions.size();
  DeviceCI.ppEnabledExtensionNames = (Extensions.size() == 0) ? nullptr : Extensions.data();

  if((Err = vkCreateDevice(PhysDev.vulkanPhysicalDevice, &DeviceCI, nullptr, &vulkanDevice)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create device with error: " + std::to_string(Err));
  }

  vkGetDeviceQueue(vulkanDevice, PhysDev.GrapicsFamily, 0, &GraphicsQueue);
}

VkFence Device::GetFence()
{
  VkResult Err;

  VkFence Ret;

  VkFenceCreateInfo FenceCI{};
  FenceCI.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  if((Err = vkCreateFence(vulkanDevice, &FenceCI, nullptr, &Ret)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create fence with error: " + std::to_string(Err));
  }

  return Ret;
}

VkSemaphore Device::GetSemaphore()
{
  VkResult Err;

  VkSemaphore Ret;

  VkSemaphoreCreateInfo SemCI{};
  SemCI.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  if((Err = vkCreateSemaphore(vulkanDevice, &SemCI, nullptr, &Ret)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create semaphore with error: " + std::to_string(Err));
  }

  return Ret;
}

VkBuffer Device::GetBuffer(PhysicalDevice& PDev, VkDeviceMemory& outMemory, uint32_t Size, VkBufferUsageFlags Usage)
{
  VkResult Err;

  VkBuffer Ret;

  VkBufferCreateInfo BufferCI{};
  BufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  BufferCI.size = Size;
  BufferCI.usage = Usage;
  BufferCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if((Err = vkCreateBuffer(vulkanDevice, &BufferCI, nullptr, &Ret)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create buffer with error: " + std::to_string(Err));
  }

  VkMemoryRequirements MemReq;
  vkGetBufferMemoryRequirements(vulkanDevice, Ret, &MemReq);

  VkMemoryAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  AllocInfo.allocationSize = MemReq.size;
  AllocInfo.memoryTypeIndex = PDev.Host;

  if((Err = vkAllocateMemory(vulkanDevice, &AllocInfo, nullptr, &outMemory)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate memory for buffer with error: " + std::to_string(Err));
  }

  if((Err = vkBindBufferMemory(vulkanDevice, Ret, outMemory, 0)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to bind buffer memory with error: " + std::to_string(Err));
  }

  return Ret;
}

void Swapchain::Create(Instance& Inst, PhysicalDevice& PDev, Device& Dev, VkExtent2D& Resolution)
{
  VkResult Err;

  // Query available formats for RGB formats
  {
    uint32_t FormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(PDev.vulkanPhysicalDevice, Inst.Surface, &FormatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> Formats(FormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(PDev.vulkanPhysicalDevice, Inst.Surface, &FormatCount, Formats.data());

    for(uint32_t i = 0; i < FormatCount; i++)
    {
      if(Formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
        SwapchainFormat = Formats[i];
        break;
      }
    }
  }

  // Create present swapchain
  {
    VkSwapchainCreateInfoKHR SwapchainCI{};
    SwapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    SwapchainCI.pNext = nullptr;

    // Triple Buffered, this number must be equal to, or larger than the minImageCount from the vkGetPhysicalDeviceSurfaceCapabilitiesKHR()
    SwapchainCI.minImageCount = 3;
    SwapchainCI.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    SwapchainCI.imageExtent = Resolution;
    SwapchainCI.imageFormat = SwapchainFormat.format;
    SwapchainCI.imageColorSpace = SwapchainFormat.colorSpace;
    SwapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    SwapchainCI.imageArrayLayers = 1;

    SwapchainCI.surface = Inst.Surface;
    SwapchainCI.clipped = VK_TRUE;
    SwapchainCI.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    SwapchainCI.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    if((Err = vkCreateSwapchainKHR(Dev.vulkanDevice, &SwapchainCI, nullptr, &vulkanSwapchain)) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create swapchain with error: " + std::to_string(Err));
    }
  }

  // Get Swapchain Images
  {
    uint32_t ImageCount;
    vkGetSwapchainImagesKHR(Dev.vulkanDevice, vulkanSwapchain, &ImageCount, nullptr);
    SwapchainImages.resize(ImageCount);
    vkGetSwapchainImagesKHR(Dev.vulkanDevice, vulkanSwapchain, &ImageCount, SwapchainImages.data());
  }

  // Make Swapchain image Views
  {
    SwapchainImageViews.resize(SwapchainImages.size());

    for(uint32_t i = 0; i < SwapchainImages.size(); i++)
    {
      VkImageViewCreateInfo ViewCI{};
      ViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      ViewCI.format = SwapchainFormat.format;
      ViewCI.image = SwapchainImages[i];
      ViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;

      ViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
      ViewCI.components.g = VK_COMPONENT_SWIZZLE_G;
      ViewCI.components.b = VK_COMPONENT_SWIZZLE_B;
      ViewCI.components.a = VK_COMPONENT_SWIZZLE_A;

      ViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      ViewCI.subresourceRange.layerCount = 1;
      ViewCI.subresourceRange.levelCount = 1;
      ViewCI.subresourceRange.baseMipLevel = 0;
      ViewCI.subresourceRange.baseArrayLayer = 0;

      if((Err = vkCreateImageView(Dev.vulkanDevice, &ViewCI, nullptr, &SwapchainImageViews[i])) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create swapchain image view" + std::to_string(Err));
      }
    }
  }
}

void Swapchain::CreateFrameBuffers(RenderPass& pRP, Device& Dev, VkExtent2D& Extent)
{
  VkResult Err;

  FrameBuffers.resize(SwapchainImages.size());

  for(uint32_t i = 0; i < SwapchainImages.size(); i++)
  {
    VkFramebufferCreateInfo FrameBufferCI{};
    FrameBufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    FrameBufferCI.attachmentCount = 1;
    FrameBufferCI.pAttachments = &SwapchainImageViews[i];
    FrameBufferCI.width = Extent.width;
    FrameBufferCI.height = Extent.height;
    FrameBufferCI.layers = 1;
    FrameBufferCI.renderPass = pRP.vulkanRenderPass;

    if((Err = vkCreateFramebuffer(Dev.vulkanDevice, &FrameBufferCI, nullptr, &FrameBuffers[i])) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create framebuffer with error: " + std::to_string(Err));
    }
  }
}

void RenderPass::Create(Swapchain& Swap, Device& Dev, VkExtent2D& Extent)
{
  VkResult Err;

  PassExtent = Extent;

  VkAttachmentDescription ColorDesc{};

  // ColorDesc represents the swapchain image we got during swapchain creation. it will be index 0 of the framebuffers same goes for VkAttachmentReference Structures
  {
    ColorDesc.format = Swap.SwapchainFormat.format;
    ColorDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    ColorDesc.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ColorDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    ColorDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ColorDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ColorDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ColorDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  }

  VkAttachmentReference ColorRef{};

  // Render Pass
  {
    ColorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    ColorRef.attachment = 0;

    Subpass.colorAttachmentCount = 1;
    Subpass.pColorAttachments = &ColorRef;
    Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  }

  VkRenderPassCreateInfo RenderpassInfo{};
  RenderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

  RenderpassInfo.subpassCount = 1;
  RenderpassInfo.pSubpasses = &Subpass;

  RenderpassInfo.attachmentCount = 1;
  RenderpassInfo.pAttachments = &ColorDesc;

  if((Err = vkCreateRenderPass(Dev.vulkanDevice, &RenderpassInfo, nullptr, &vulkanRenderPass)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create renderpass with error: " + std::to_string(Err));
  }
}

void RenderPass::Begin(VkFramebuffer& RenderTarget, VkCommandBuffer& cmdBuff)
{
  VkClearValue Clear;

  for(uint8_t i = 0; i < 4; i++)
  {
    Clear.color.int32[i] = 0;
    Clear.color.uint32[i] = 0;
    Clear.color.float32[i] = 0;
  }

  VkRenderPassBeginInfo BeginInfo{};
  BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  BeginInfo.renderArea.extent = PassExtent;
  BeginInfo.renderArea.offset = {0, 0};
  BeginInfo.renderPass = vulkanRenderPass;
  BeginInfo.framebuffer = RenderTarget;
  BeginInfo.clearValueCount = 1;
  BeginInfo.pClearValues = &Clear;

  vkCmdBeginRenderPass(cmdBuff, &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPass::End(VkCommandBuffer& cmdBuff)
{
  vkCmdEndRenderPass(cmdBuff);
}

void DescriptorPool::CreatePool(Device& Dev, uint32_t DescriptorCount)
{
  VkResult Err;

  VkDescriptorPoolCreateInfo PoolCI{};
  PoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  PoolCI.maxSets = DescriptorCount;
  PoolCI.poolSizeCount = Sizes.size();
  PoolCI.pPoolSizes = Sizes.data();

  if((Err = vkCreateDescriptorPool(Dev.vulkanDevice, &PoolCI, nullptr, &vulkanDescriptorPool)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor pool with error: " + std::to_string(Err));
  }
}

void DescriptorPool::PushLayout(VkDescriptorSetLayout Layout, VkDescriptorPoolSize Size)
{
  Layouts.push_back(Layout);
  Sizes.push_back(Size);
}

VkDescriptorSet DescriptorPool::GetDescriptor(Device& Dev, uint32_t LayoutIndex)
{
  VkResult Err;

  VkDescriptorSet Ret;

  VkDescriptorSetAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  AllocInfo.descriptorPool = vulkanDescriptorPool;
  AllocInfo.descriptorSetCount = 1;
  AllocInfo.pSetLayouts = &Layouts[LayoutIndex];

  if((Err = vkAllocateDescriptorSets(Dev.vulkanDevice, &AllocInfo, &Ret)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate descriptor set with error: " + std::to_string(Err));
  }

  return Ret;
}

void Pipeline::Create(RenderPass& pRP, Device& Dev, DescriptorPool& Pool, VkExtent2D& Extent)
{
  VkResult Err;

  // Create shaders
  {
    uint32_t VertexSize;
    std::ifstream Vertex;
    Vertex.open("Vert.spv", std::ios::binary | std::ios::ate);

    if(!Vertex.is_open())
    {
      throw std::runtime_error("Failed to open Vert.spv");
    }

    uint32_t FragmentSize;
    std::ifstream Fragment;
    Fragment.open("Frag.spv", std::ios::binary | std::ios::ate);

    if(!Fragment.is_open())
    {
      throw std::runtime_error("Failed to open Frag.spv");
    }
    VertexSize = Vertex.tellg();
    FragmentSize = Fragment.tellg();

    Vertex.seekg(0);
    Fragment.seekg(0);

    std::vector<char> VertexCode(VertexSize);
    std::vector<char> FragmentCode(FragmentSize);

    Vertex.read(VertexCode.data(), VertexSize);
    Fragment.read(FragmentCode.data(), FragmentSize);

    VkShaderModuleCreateInfo VertModCI{};
    VertModCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    VertModCI.codeSize = VertexSize;
    VertModCI.pCode = reinterpret_cast<const uint32_t*>(VertexCode.data());

    VkShaderModuleCreateInfo FragmentCI{};
    FragmentCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    FragmentCI.codeSize = FragmentSize;
    FragmentCI.pCode = reinterpret_cast<const uint32_t*>(FragmentCode.data());

    if((Err = vkCreateShaderModule(Dev.vulkanDevice, &VertModCI, nullptr, &VertexModule)) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create with error: " + std::to_string(Err));
    }

    if((Err = vkCreateShaderModule(Dev.vulkanDevice, &FragmentCI, nullptr, &FragmentModule)) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create with error: " + std::to_string(Err));
    }
  }

  VkPipelineLayoutCreateInfo LayCI{};
  LayCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  LayCI.setLayoutCount = Pool.Layouts.size();
  LayCI.pSetLayouts = Pool.Layouts.data();
  LayCI.pushConstantRangeCount = 0;

  if((Err = vkCreatePipelineLayout(Dev.vulkanDevice, &LayCI, nullptr, &PipelineLayout)) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create pipeline layout with error: " + std::to_string(Err));
  }

  VkViewport Viewport;
  Viewport.width = Extent.width;
  Viewport.height = Extent.height;
  Viewport.x = 0;
  Viewport.y = 0;
  Viewport.minDepth = 0;
  Viewport.maxDepth = 1;

  VkRect2D Rect;
  Rect.extent.width = Extent.width;
  Rect.extent.height = Extent.height;
  Rect.offset = {0, 0};

  VkPipelineViewportStateCreateInfo ViewportState{};
  ViewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  ViewportState.scissorCount = 1;
  ViewportState.pScissors = &Rect;
  ViewportState.viewportCount = 1;
  ViewportState.pViewports = &Viewport;

  VkPipelineInputAssemblyStateCreateInfo InputAssembler{};
  InputAssembler.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  InputAssembler.primitiveRestartEnable = VK_FALSE;
  InputAssembler.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  VkPipelineVertexInputStateCreateInfo VertexInputState{};
  VertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  VertexInputState.vertexBindingDescriptionCount = 0;
  VertexInputState.vertexAttributeDescriptionCount = 0;

  VkPipelineShaderStageCreateInfo VertexStage{};
  VertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  VertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  VertexStage.pName = "main";
  VertexStage.module = VertexModule;

  VkPipelineShaderStageCreateInfo FragmentStage{};
  FragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  FragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  FragmentStage.pName = "main";
  FragmentStage.module = FragmentModule;

  VkPipelineShaderStageCreateInfo ShaderStages[2] = {VertexStage, FragmentStage};

  VkPipelineColorBlendAttachmentState ColorAttachment{};
  ColorAttachment.blendEnable = VK_FALSE;
  ColorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo ColorBlendState{};
  ColorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  ColorBlendState.logicOpEnable = VK_FALSE;
  ColorBlendState.attachmentCount = 1;
  ColorBlendState.pAttachments = &ColorAttachment;
  ColorBlendState.blendConstants[0] = 0.f;
  ColorBlendState.blendConstants[1] = 0.f;
  ColorBlendState.blendConstants[2] = 0.f;
  ColorBlendState.blendConstants[3] = 0.f;

  VkPipelineRasterizationStateCreateInfo RasterState{};
  RasterState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  RasterState.cullMode = VK_CULL_MODE_BACK_BIT;
  RasterState.frontFace = VK_FRONT_FACE_CLOCKWISE;
  RasterState.polygonMode = VK_POLYGON_MODE_FILL;
  RasterState.lineWidth = 1.f;
  RasterState.depthBiasEnable = VK_FALSE;
  RasterState.rasterizerDiscardEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo MultiSample{};
  MultiSample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  MultiSample.sampleShadingEnable = VK_FALSE;
  MultiSample.alphaToOneEnable = VK_FALSE;
  MultiSample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  MultiSample.alphaToCoverageEnable = VK_FALSE;

  VkGraphicsPipelineCreateInfo GraphicsPipe{};
  GraphicsPipe.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  GraphicsPipe.renderPass = pRP.vulkanRenderPass;
  GraphicsPipe.subpass = 0;
  GraphicsPipe.layout = PipelineLayout;
  GraphicsPipe.stageCount = 2;
  GraphicsPipe.pStages = ShaderStages;
  GraphicsPipe.pMultisampleState = &MultiSample;
  GraphicsPipe.pColorBlendState = &ColorBlendState;
  GraphicsPipe.pViewportState = &ViewportState;
  GraphicsPipe.pVertexInputState = &VertexInputState;
  GraphicsPipe.pInputAssemblyState = &InputAssembler;
  GraphicsPipe.pRasterizationState = &RasterState;

  if((Err = vkCreateGraphicsPipelines(Dev.vulkanDevice, VK_NULL_HANDLE, 1, &GraphicsPipe, nullptr, &vulkanPipeline)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create graphics pipeline with error: " + std::to_string(Err));
  }
}

void Pipeline::Bind(VkCommandBuffer& cmdBuff)
{
  vkCmdBindPipeline(cmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline);
}

void CommandPool::Create(PhysicalDevice& PDev, Device& Dev)
{
  VkResult Err;

  VkCommandPoolCreateInfo PoolCI{};
  PoolCI.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  PoolCI.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  PoolCI.queueFamilyIndex = PDev.GrapicsFamily;

  if((Err = vkCreateCommandPool(Dev.vulkanDevice, &PoolCI, nullptr, &vulkanCommandPool)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create command pool with error: " + std::to_string(Err));
  }
}

VkCommandBuffer CommandPool::GetCommandBuffer(Device& Dev)
{
  VkResult Err;

  VkCommandBuffer Ret;

  VkCommandBufferAllocateInfo AllocInfo{};
  AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  AllocInfo.commandPool = vulkanCommandPool;
  AllocInfo.commandBufferCount = 1;
  AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  if((Err = vkAllocateCommandBuffers(Dev.vulkanDevice, &AllocInfo, &Ret)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create command buffer with error: " + std::to_string(Err));
  }

  return Ret;
}

