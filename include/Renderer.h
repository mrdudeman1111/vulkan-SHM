#include "Context.h"

#include <sys/wait.h>

#include "sail/sail.h"

class Renderer
{
public:
  Renderer();

  void BeginRendering();
  void EndRendering(VkFence* pFence);

  void BeginCommand();
  void EndCommand(VkFence* Fence);

  void Present();

  /*
    in  Usage: VkImageUsageFlags with bits for each use case this image will be used under
    out Format: VkFormat will be filled with the image format.
  */
  void ImportSharedImage(sail_image* sailImage, VkImageUsageFlags Usage, VkImage& Image, VkDeviceMemory& Memory, VkFormat Format);
  void CreateImage(VkImage& Image, VkDeviceMemory& Memory, VkExtent3D Extent, VkImageUsageFlags Usage, VkFormat Format, void* pNext);
  void CreateView(VkImageView& View, VkImage& Image, VkFormat Format, VkImageAspectFlags Aspect);

  void WaitForFence(VkFence& pFence);

  // render settings
    VkExtent2D RenderExtent;
    VulkanContext RenderContext;

  // memory
    VkBuffer TransferBuffer;
    VkDeviceMemory TransferMemory;

  // Rendering
  VkCommandBuffer cmdBuffer;

  // sync
    uint32_t ImageIndex;

    // fence for synchronizing Frame buffer access
    VkFence AcquireFence;
    // fence for synchronizing Command buffer execution
    VkFence ExecFence;
    // semaphore for synchronizing rendering operations with present
    VkSemaphore PresentSem;

protected:
  void InitVulkan(bool bWindow, VkExtent2D* inExtent);
  void InitFrameBuffers();
  void InitDescriptors();
  void InitPipeline();

  void AddDescriptor(VkDescriptorSetLayoutCreateInfo& LayoutCI);
  VkDescriptorSet GetDescriptor(uint32_t LayoutIndex);
  void UpdateDescriptor(VkWriteDescriptorSet Write);
};

class Exporter : public Renderer
{
  public:
    // the pointer is void because it's one of two things, a window HANDLE, or a linux HANDLE
    void Init();
    void LoadImage(const char* ImagePath);

    VkImage Image;
    VkDeviceMemory ImageMemory;
};

class Importer : public Renderer
{
  public:
    void Init(VkExtent2D& Extent);

    void ImportImage(uint32_t Handle);

    VkImage Image;
    VkDeviceMemory ImageMemory;
};

