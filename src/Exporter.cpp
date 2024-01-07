#include "Renderer.h"
#include <algorithm>
#include <cstring>
#include <vulkan/vulkan_core.h>

void Exporter::Init()
{
  InitVulkan(false, nullptr);
  RenderContext.cmdPool.Create(RenderContext.PDev, RenderContext.Dev);

  TransferBuffer = RenderContext.Dev.GetBuffer(RenderContext.PDev, TransferMemory, 28000000, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  cmdBuffer = RenderContext.cmdPool.GetCommandBuffer(RenderContext.Dev);
  ExecFence = RenderContext.Dev.GetFence();
}

void Exporter::LoadImage(const char* ImagePath)
{
  sail_image* sailImage;
  sail_load_from_file(ImagePath, &sailImage);

  VkExtent2D Extent { sailImage->width, sailImage->height };

  ImportImage(sailImage, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, Image, ImageMemory, VK_FORMAT_R8G8B8_UNORM);

  /*
  void* pTransferMemory;
  vkMapMemory(RenderContext.Dev.vulkanDevice, TransferMemory, 0, 28000000, 0, &pTransferMemory);
    memcpy(pTransferMemory, sailImage->pixels, (sailImage->width+sailImage->height)*4);
  vkUnmapMemory(RenderContext.Dev.vulkanDevice, TransferMemory);

  VkExternalMemoryImageCreateInfo ExtImageInfo{};
  ExtImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
  ExtImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

  CreateImage(Image, ImageMemory, VkExtent3D{Extent.width, Extent.height, 1}, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_FORMAT_R8G8B8A8_SRGB, &ExtImageInfo);

  // transfer to transfer dst
  {
    VkImageMemoryBarrier MemBarrier{};
    MemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    MemBarrier.image = Image;

    MemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    MemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    MemBarrier.srcAccessMask = VK_ACCESS_NONE;
    MemBarrier.dstAccessMask = VK_ACCESS_NONE;

    MemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    MemBarrier.subresourceRange.levelCount = 1;
    MemBarrier.subresourceRange.layerCount = 1;
    MemBarrier.subresourceRange.baseMipLevel = 0;
    MemBarrier.subresourceRange.baseArrayLayer = 0;

    BeginCommand();
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &MemBarrier);
    EndCommand(&ExecFence);
  }

  // Transfer
  {
    VkBufferImageCopy Copy{};
    Copy.imageExtent = VkExtent3D{sailImage->width, sailImage->height, 1};
    Copy.imageOffset = VkOffset3D{0, 0, 0};

    Copy.bufferOffset = 0;

    Copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    Copy.imageSubresource.layerCount = 1;
    Copy.imageSubresource.mipLevel = 0;
    Copy.imageSubresource.baseArrayLayer = 0;

    WaitForFence(ExecFence);

    BeginCommand();
      vkCmdCopyBufferToImage(cmdBuffer, TransferBuffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Copy);
    EndCommand(&ExecFence);
  }

  // Transfer to color attachment
  {
    VkImageMemoryBarrier MemBarrier{};
    MemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

    MemBarrier.image = Image;
    MemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    MemBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    MemBarrier.srcAccessMask = VK_ACCESS_NONE;
    MemBarrier.dstAccessMask = VK_ACCESS_NONE;

    MemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    MemBarrier.subresourceRange.layerCount = 1;
    MemBarrier.subresourceRange.levelCount = 1;
    MemBarrier.subresourceRange.baseMipLevel = 0;
    MemBarrier.subresourceRange.baseArrayLayer = 0;

    WaitForFence(ExecFence);

    BeginCommand();
      vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &MemBarrier);
    EndCommand(&ExecFence);

    WaitForFence(ExecFence);
  }
  */
}

