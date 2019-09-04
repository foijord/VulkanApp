#pragma once

#include <Innovator/Wrapper.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <Windows.h>
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#endif

class VulkanSurface {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSurface)
  VulkanSurface() = delete;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
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

#elif defined(VK_USE_PLATFORM_XCB_KHR)
  VulkanSurface(std::shared_ptr<VulkanInstance> vulkan,
                xcb_window_t window,
                xcb_connection_t * connection) : 
    vulkan(std::move(vulkan))
  {
    VkXcbSurfaceCreateInfoKHR create_info {
      VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, // sType 
      nullptr,                                       // pNext
      0,                                             // flags (reserved for future use)
      connection,                                    // connection
      window,                                        // window
    };

    THROW_ON_ERROR(vkCreateXcbSurfaceKHR(this->vulkan->instance, &create_info, nullptr, &this->surface));
  }
#endif

  ~VulkanSurface()
  {
    vkDestroySurfaceKHR(this->vulkan->instance, this->surface, nullptr);
  }

  VkSurfaceFormatKHR getSupportedSurfaceFormat(std::shared_ptr<VulkanDevice> device, VkFormat format)
  {
    std::vector<VkSurfaceFormatKHR> surface_formats =
    this->vulkan->getPhysicalDeviceSurfaceFormats(device->physical_device.device, this->surface);

    for (VkSurfaceFormatKHR surface_format : surface_formats) {
      if (surface_format.format == format) {
        return surface_format;
      }
    }
    throw std::runtime_error("surface format not supported!");
  }

  void checkPresentModeSupport(std::shared_ptr<VulkanDevice> device, VkPresentModeKHR present_mode)
  {
    std::vector<VkPresentModeKHR> present_modes =
        this->vulkan->getPhysicalDeviceSurfacePresentModes(device->physical_device.device, this->surface);

    if (std::find(present_modes.begin(), present_modes.end(), present_mode) == present_modes.end()) {
      throw std::runtime_error("surface does not support present mode");
    }
  }
    
  VkSurfaceCapabilitiesKHR getSurfaceCapabilities(std::shared_ptr<VulkanDevice> device)
  {
    return this->vulkan->getPhysicalDeviceSurfaceCapabilities(device->physical_device.device, this->surface);
  }


  std::shared_ptr<VulkanInstance> vulkan;
  VkSurfaceKHR surface { nullptr };
};
