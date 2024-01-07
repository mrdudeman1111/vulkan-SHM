#include <cstdint>
#include <iostream>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "Tools.h"
#include "GLFW/glfw3.h"
#include "Renderer.h"

Exporter Exp;

int main()
{
  Exp.Init();
  Exp.LoadImage("input.png");

  VkExportMemoryAllocateInfo ImageExport{};
  ImageExport.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
  ImageExport.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

  VkExternalMemoryImageCreateInfo ImageInfo{};
  ImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;



  VkDescriptorSetLayoutBinding Binding{};
  Binding.binding = 0;
  Binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  Binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  Binding.descriptorCount = 1;

  VkDescriptorSetLayoutCreateInfo LayoutCI{};
  LayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  LayoutCI.bindingCount = 1;
  LayoutCI.pBindings = &Binding;

  /*
  MainRenderer.AddDescriptor(LayoutCI);

  // Init rendering structures
  MainRenderer.InitDescriptors();
  MainRenderer.InitPipeline();

    VkFormat Format;
    VkDeviceMemory Memory;
    VkImage Texture;
    VkImageView TextureView;
    VkDescriptorSet ImgDescriptor;

  // image
  // image memory
  // image view
  // image sampler
  // descriptor set

  MainRenderer.ImportImage(Image, VK_IMAGE_USAGE_SAMPLED_BIT, Texture, Memory, Format); MainRenderer.CreateView(TextureView, Texture, Format, VK_IMAGE_ASPECT_COLOR_BIT);
  ImgDescriptor = MainRenderer.GetDescriptor(0);

  VkSamplerCreateInfo SamplerCI{};
  VkSampler Sampler;

  // create sampler
  {
    SamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerCI.compareOp = VK_COMPARE_OP_EQUAL;

    SamplerCI.minLod = 1.f;
    SamplerCI.maxLod = 1.f;

    SamplerCI.minFilter = VK_FILTER_NEAREST;
    SamplerCI.magFilter = VK_FILTER_NEAREST;

    SamplerCI.maxAnisotropy = 1.f;
    SamplerCI.mipLodBias = 1.f;
    SamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    SamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    SamplerCI.compareEnable = VK_FALSE;
    SamplerCI.compareOp = VK_COMPARE_OP_EQUAL;
    SamplerCI.anisotropyEnable = VK_FALSE;
    SamplerCI.unnormalizedCoordinates = VK_FALSE;

    vkCreateSampler(MainRenderer.RenderContext.Dev.vulkanDevice, &SamplerCI, nullptr, &Sampler);
  }

  // Update descriptor
  {
    VkDescriptorImageInfo DescImg{};
    DescImg.sampler = Sampler;
    DescImg.imageView = TextureView;
    DescImg.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet Write{};
    Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    Write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    Write.descriptorCount = 1;
    Write.pImageInfo = &DescImg;
    Write.dstSet = ImgDescriptor;
    Write.dstBinding = 0;
    Write.dstArrayElement = 0;

    MainRenderer.UpdateDescriptor(Write);
  }

  while(!glfwWindowShouldClose(MainRenderer.RenderContext.Inst.Window))
  {
    glfwPollEvents();

      MainRenderer.BeginRendering();
        vkCmdBindDescriptorSets(MainRenderer.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, MainRenderer.RenderContext.Pipe.PipelineLayout, 0, 1, &ImgDescriptor, 0, nullptr);
        vkCmdDraw(MainRenderer.cmdBuffer, 6, 1, 0, 0);
      MainRenderer.EndRendering(&MainRenderer.ExecFence);
    MainRenderer.Present();

    MainRenderer.WaitForFence(MainRenderer.ExecFence);
  }
  */

  std::cout << "Successful run\n";
}

