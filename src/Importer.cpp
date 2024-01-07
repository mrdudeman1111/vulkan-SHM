#include "Renderer.h"

void Importer::Init(VkExtent2D& Extent)
{
  InitVulkan(true, &Extent);
  InitFrameBuffers();
}

void Importer::ImportImage(uint32_t Handle)
{
  VkMemoryFdPropertiesKHR MemProps{};

  PFN_vkGetMemoryFdPropertiesKHR* GetMemoryProperties = (PFN_vkGetMemoryFdPropertiesKHR*)vkGetInstanceProcAddr(RenderContext.Inst.vulkanInstance, "vkGetMemoryFdPropertiesKHR");

  (*GetMemoryProperties)(RenderContext.Dev.vulkanDevice, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT, Handle, &MemProps);

  VkExternalImageFormatPropertiesKHR ExtProps{};
  ExtProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES_KHR;

  VkImportMemoryFdInfoKHR Import{};
  Import.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
  Import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
  Import.fd = Handle;

}

