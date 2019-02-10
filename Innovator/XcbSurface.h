#pragma once

#include <Innovator/Vulkan/Wrapper.h>
#include <xcb/xcb.h>

class VulkanSurface {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSurface)
  VulkanSurface() = delete;

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

  ~VulkanSurface()
  {
    vkDestroySurfaceKHR(this->vulkan->instance, this->surface, nullptr);
  }

  std::shared_ptr<VulkanInstance> vulkan;
  VkSurfaceKHR surface { nullptr };
};
