#include "Renderer.h"
#include "Tools.h"

#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <string>

#include <sail/sail/sail.h>
#include <sail/sail-manip/sail-manip.h>
#include <vulkan/vulkan_core.h>

Renderer::Renderer()
{
  cmdBuffer = VK_NULL_HANDLE;
  AcquireFence = VK_NULL_HANDLE;
  ExecFence = VK_NULL_HANDLE;
  PresentSem = VK_NULL_HANDLE;
}

void Renderer::InitVulkan(bool bWindow, VkExtent2D* inExtent)
{
  if(inExtent) RenderExtent = *inExtent;

  RenderContext.Inst.Layers.push_back("VK_LAYER_KHRONOS_validation");

  RenderContext.Inst.Extensions.push_back("VK_KHR_get_physical_device_properties2");
  RenderContext.Inst.Extensions.push_back("VK_KHR_external_memory_capabilities");

  //RenderContext.Dev.Extensions.push_back("VK_KHR_surface");
  RenderContext.Dev.Extensions.push_back("VK_KHR_swapchain");
  RenderContext.Dev.Extensions.push_back("VK_KHR_external_memory");

#ifdef WIN32
  RenderContext.Dev.Extensions.push_back("VK_KHR_external_memory_win32");
#else
  RenderContext.Dev.Extensions.push_back("VK_KHR_external_memory_fd");
#endif

  RenderContext.Inst.Create(
      (inExtent == nullptr) ? RenderExtent.width : 0
      , (inExtent == nullptr) ? RenderExtent.height : 0
      , bWindow);

  RenderContext.PDev.Create(RenderContext.Inst);

  RenderContext.Dev.Create(RenderContext.PDev);
}

void Renderer::InitFrameBuffers()
{
  RenderContext.Swap.Create(RenderContext.Inst, RenderContext.PDev, RenderContext.Dev, RenderExtent);

  RenderContext.RPass.Create(RenderContext.Swap, RenderContext.Dev, RenderExtent);

  RenderContext.Swap.CreateFrameBuffers(RenderContext.RPass, RenderContext.Dev, RenderExtent);

  RenderContext.cmdPool.Create(RenderContext.PDev, RenderContext.Dev);

  cmdBuffer = RenderContext.cmdPool.GetCommandBuffer(RenderContext.Dev);
  AcquireFence = RenderContext.Dev.GetFence();
  ExecFence = RenderContext.Dev.GetFence();
  PresentSem = RenderContext.Dev.GetSemaphore();

  TransferBuffer = RenderContext.Dev.GetBuffer(RenderContext.PDev, TransferMemory, 512000000, VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

  // we convert all our images to a layout that is useable by the renderpass
  for(uint32_t i = 0; i < RenderContext.Swap.SwapchainImages.size(); i++)
  {
    VkImageMemoryBarrier ImageBarrier{};
    ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImageBarrier.image = RenderContext.Swap.SwapchainImages[i];
    ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    ImageBarrier.srcAccessMask = VK_ACCESS_NONE;
    ImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageBarrier.subresourceRange.layerCount = 1;
    ImageBarrier.subresourceRange.baseArrayLayer = 0;
    ImageBarrier.subresourceRange.levelCount = 1;
    ImageBarrier.subresourceRange.baseMipLevel = 0;

    BeginCommand();
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier);
    EndCommand(&ExecFence);

    vkWaitForFences(RenderContext.Dev.vulkanDevice, 1, &ExecFence, VK_TRUE, UINT64_MAX);
    vkResetFences(RenderContext.Dev.vulkanDevice, 1, &ExecFence);
  }

}

void Renderer::AddDescriptor(VkDescriptorSetLayoutCreateInfo& LayoutCI)
{
  VkResult Err;

  VkDescriptorSetLayout Layout;

  if((Err = vkCreateDescriptorSetLayout(RenderContext.Dev.vulkanDevice, &LayoutCI, nullptr, &Layout)) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout with error: " + std::to_string(Err));
  }

  VkDescriptorPoolSize Size{};
  Size.type = LayoutCI.pBindings[0].descriptorType;
  Size.descriptorCount = LayoutCI.pBindings[0].descriptorCount;

  RenderContext.DescPool.PushLayout(Layout, Size);
}

VkDescriptorSet Renderer::GetDescriptor(uint32_t LayoutIndex)
{
  return RenderContext.DescPool.GetDescriptor(RenderContext.Dev, LayoutIndex);
}

void Renderer::UpdateDescriptor(VkWriteDescriptorSet Write)
{
  vkUpdateDescriptorSets(RenderContext.Dev.vulkanDevice, 1, &Write, 0, nullptr);
}

void Renderer::InitDescriptors()
{
  RenderContext.DescPool.CreatePool(RenderContext.Dev, 1);
}

void Renderer::InitPipeline()
{
  RenderContext.Pipe.Create(RenderContext.RPass, RenderContext.Dev, RenderContext.DescPool, RenderExtent);
}

void Renderer::BeginCommand()
{
  VkCommandBufferBeginInfo BeginInfo{};
  BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(cmdBuffer, &BeginInfo);
}

void Renderer::EndCommand(VkFence* pFence)
{
  vkEndCommandBuffer(cmdBuffer);

  VkSubmitInfo SubmitInfo{};
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &cmdBuffer;

  vkQueueSubmit(RenderContext.Dev.GraphicsQueue, 1, &SubmitInfo, (pFence == nullptr) ? nullptr : *pFence);
}

void Renderer::BeginRendering()
{
  vkAcquireNextImageKHR(RenderContext.Dev.vulkanDevice, RenderContext.Swap.vulkanSwapchain, UINT64_MAX, VK_NULL_HANDLE, AcquireFence, &ImageIndex);
  vkWaitForFences(RenderContext.Dev.vulkanDevice, 1, &AcquireFence, VK_TRUE, UINT64_MAX);
  vkResetFences(RenderContext.Dev.vulkanDevice, 1, &AcquireFence);

  BeginCommand();
  RenderContext.RPass.Begin(RenderContext.Swap.FrameBuffers[ImageIndex], cmdBuffer);
  RenderContext.Pipe.Bind(cmdBuffer);
}

void Renderer::EndRendering(VkFence* pFence)
{
  vkCmdEndRenderPass(cmdBuffer);
  vkEndCommandBuffer(cmdBuffer);

  VkSubmitInfo SubmitInfo{};
  SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  SubmitInfo.commandBufferCount = 1;
  SubmitInfo.pCommandBuffers = &cmdBuffer;
  SubmitInfo.signalSemaphoreCount = 1;
  SubmitInfo.pSignalSemaphores = &PresentSem;

  vkQueueSubmit(RenderContext.Dev.GraphicsQueue, 1, &SubmitInfo, (pFence == nullptr) ? nullptr : *pFence);
}

void Renderer::Present()
{
  VkPresentInfoKHR PresentInfo{};
  PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  PresentInfo.waitSemaphoreCount = 1;
  PresentInfo.pWaitSemaphores = &PresentSem;
  PresentInfo.swapchainCount = 1;
  PresentInfo.pSwapchains = &RenderContext.Swap.vulkanSwapchain;
  PresentInfo.pImageIndices = &ImageIndex;

  vkQueuePresentKHR(RenderContext.Dev.GraphicsQueue, &PresentInfo);
}

void Renderer::ImportSharedImage(sail_image* sailImage, VkImageUsageFlags Usage, VkImage& Image, VkDeviceMemory& Memory, VkFormat Format)
{
  VkResult Err;

  void* pTransfer;
  vkMapMemory(RenderContext.Dev.vulkanDevice, TransferMemory, 0, 512000000, 0, &pTransfer);
    memcpy(pTransfer, sailImage->pixels, sailImage->height*sailImage->width*4);
  vkUnmapMemory(RenderContext.Dev.vulkanDevice, TransferMemory);

  VkExternalMemoryImageCreateInfoKHR ExtImage{};
  ExtImage.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
  ExtImage.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

  std::cout << "Importing resolution " << sailImage->width << 'x' << sailImage->height << '\n';
  CreateImage(Image, Memory, VkExtent3D{sailImage->width, sailImage->height, 1}, (Usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) ? Usage : Usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_FORMAT_R8G8B8A8_SRGB, &ExtImage);
  Format = VK_FORMAT_R8G8B8A8_SRGB;

      /* Todo */
  /*
  allocate and bind to memory
  */

  // change image layout for transfer
  {
    VkImageMemoryBarrier Barrier{};
    Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    Barrier.image = Image;

    Barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    Barrier.srcAccessMask = VK_ACCESS_NONE;
    Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    Barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Barrier.subresourceRange.layerCount = 1;
    Barrier.subresourceRange.levelCount = 1;
    Barrier.subresourceRange.baseMipLevel = 0;
    Barrier.subresourceRange.baseArrayLayer = 0;

    BeginCommand();
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &Barrier);
    EndCommand(&ExecFence);

    WaitForFence(ExecFence);
  }

  // copy buffer to image
  {
    VkBufferImageCopy ImgCopy{};

    ImgCopy.imageExtent = VkExtent3D{sailImage->width, sailImage->height, 1};
    ImgCopy.imageOffset = {0, 0, 0};
    ImgCopy.bufferOffset = 0;

    ImgCopy.imageSubresource.mipLevel = 0;
    ImgCopy.imageSubresource.layerCount = 1;
    ImgCopy.imageSubresource.baseArrayLayer = 0;
    ImgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    BeginCommand();
      vkCmdCopyBufferToImage(cmdBuffer, TransferBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &ImgCopy);
    EndCommand(&ExecFence);

    WaitForFence(ExecFence);
  }

  // Transfer to shader-compatible texture Layout
  {
    VkImageMemoryBarrier ImageBarrier{};
    ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    ImageBarrier.image = Image;

    ImageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    ImageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    ImageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    ImageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    ImageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    ImageBarrier.subresourceRange.baseArrayLayer = 0;
    ImageBarrier.subresourceRange.baseMipLevel = 0;
    ImageBarrier.subresourceRange.layerCount = 1;
    ImageBarrier.subresourceRange.levelCount = 1;

    BeginCommand();
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &ImageBarrier);
    EndCommand(&ExecFence);

    WaitForFence(ExecFence);
  }
}

void Renderer::CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkExtent3D Extent, VkImageUsageFlags Usage, VkFormat Format, void* pNext)
{
  VkResult Err;

  VkImageCreateInfo ImageCI{};
  ImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ImageCI.pNext = pNext;
  ImageCI.extent = Extent;
  ImageCI.usage = Usage;
  ImageCI.format = Format;
  ImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
  ImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
  ImageCI.imageType = VK_IMAGE_TYPE_2D;
  ImageCI.mipLevels = 1;
  ImageCI.arrayLayers = 1;
  ImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if((Err = vkCreateImage(RenderContext.Dev.vulkanDevice, &ImageCI, nullptr, &Image)) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image with error: " + std::to_string(Err));
  }
}

void Renderer::CreateView(VkImageView& View, VkImage& Image, VkFormat Format, VkImageAspectFlags Aspect)
{
  VkResult Err;
  VkImageViewCreateInfo ViewCI{};
  ViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ViewCI.format = Format;
  ViewCI.image = Image;
  ViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;

  ViewCI.components.r = VK_COMPONENT_SWIZZLE_R;
  ViewCI.components.b = VK_COMPONENT_SWIZZLE_G;
  ViewCI.components.g = VK_COMPONENT_SWIZZLE_B;
  ViewCI.components.a = VK_COMPONENT_SWIZZLE_A;

  ViewCI.subresourceRange.aspectMask = Aspect;
  ViewCI.subresourceRange.layerCount = 1;
  ViewCI.subresourceRange.baseArrayLayer = 0;
  ViewCI.subresourceRange.levelCount = 1;
  ViewCI.subresourceRange.baseMipLevel = 0;

  if((Err = vkCreateImageView(RenderContext.Dev.vulkanDevice, &ViewCI, nullptr, &View)) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image view with error: " + std::to_string(Err));
  }
}

void Renderer::WaitForFence(VkFence& Fence)
{
  vkWaitForFences(RenderContext.Dev.vulkanDevice, 1, &Fence, VK_TRUE, UINT64_MAX);
  vkResetFences(RenderContext.Dev.vulkanDevice, 1, &Fence);
}

