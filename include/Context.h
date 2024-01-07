#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

#include "Tools.h"

struct VulkanContext
{
  public:
  // Wrappers
    Instance Inst;
    PhysicalDevice PDev;
    Device Dev;
    Swapchain Swap;
    RenderPass RPass;
    DescriptorPool DescPool;
    Pipeline Pipe;
    CommandPool cmdPool;
};

