#pragma once

#include <Innovator/Core/Vulkan/Wrapper.h>
#include <Windows.h>

class VulkanSurface {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSurface)
  VulkanSurface() = delete;

  VulkanSurface(std::shared_ptr<VulkanInstance> vulkan,
                HWND window,
                HINSTANCE hinstance) : 
    vulkan(std::move(vulkan))
  {
    VkWin32SurfaceCreateInfoKHR create_info{
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // sType 
      nullptr,                                         // pNext
      0,                                               // flags (reserved for future use)
      hinstance,                                       // hinstance 
      window,                                          // hwnd
    };

    THROW_ON_ERROR(vkCreateWin32SurfaceKHR(this->vulkan->instance, &create_info, nullptr, &this->surface));
  }

  ~VulkanSurface()
  {
    vkDestroySurfaceKHR(this->vulkan->instance, this->surface, nullptr);
  }

  std::shared_ptr<VulkanInstance> vulkan;
  VkSurfaceKHR surface { nullptr };
};
