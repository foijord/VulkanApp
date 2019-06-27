#pragma once

#include <Innovator/Misc/Defines.h>
#include <Innovator/VulkanObjects.h>
#include <Innovator/Nodes.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <Innovator/Win32Surface.h>
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <Innovator/XcbSurface.h>
#endif

#include <map>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>

class VulkanViewer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanViewer)
  VulkanViewer() = delete;

  VulkanViewer(std::shared_ptr<VulkanInstance> vulkan, 
               std::shared_ptr<VulkanDevice> device,
               std::shared_ptr<::VulkanSurface> surface, 
               std::shared_ptr<Group> scene) :
    vulkan(std::move(vulkan)),
    device(std::move(device)),
    surface(std::move(surface)),
    scene(std::move(scene))
  {
    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(this->device->physical_device.device,
                                                                        this->surface->surface,
                                                                        &this->surface_capabilities));

    this->rendermanager = std::make_unique<RenderManager>(this->vulkan, 
                                                          this->device,
                                                          this->surface_capabilities.currentExtent);

    this->rendermanager->alloc(this->scene.get());
    this->rendermanager->stage(this->scene.get());
    this->rendermanager->pipeline(this->scene.get());
    this->rendermanager->record(this->scene.get());
  }

  ~VulkanViewer()
  {
    try {
      THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void redraw()
  {
    try {
      this->rendermanager->render(this->scene.get());
      this->rendermanager->present(this->scene.get());
    }
    catch (VkException &) {
      // recreate swapchain, try again next frame
      this->resize();
    }
  }

  void resize()
  {
    // make sure all work submitted is done before we start recreating stuff
    THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));

    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(this->device->physical_device.device,
                                                                        this->surface->surface,
                                                                        &this->surface_capabilities));

    this->rendermanager->resize(this->scene.get(), this->surface_capabilities.currentExtent);
    this->rendermanager->record(this->scene.get());

    this->redraw();
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<::VulkanSurface> surface;
  VkSurfaceCapabilitiesKHR surface_capabilities{};
  std::shared_ptr<Group> scene;
  std::unique_ptr<RenderManager> rendermanager;
};
