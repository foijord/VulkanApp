#pragma once

#include <Innovator/Misc/Defines.h>
#include <Innovator/VulkanObjects.h>
#include <Innovator/Nodes.h>
#include <Innovator/Camera.h>

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

class VulkanFramebufferObject {
public:
  VulkanFramebufferObject(std::shared_ptr<VulkanInstance> vulkan,
                          std::shared_ptr<VulkanDevice> device,
                          std::shared_ptr<VulkanRenderpass> renderpass,
                          VkSurfaceKHR surface,
                          VkFormat depth_format,
                          VkSurfaceFormatKHR surface_format) :
                          device(std::move(device)),
                          renderpass(std::move(renderpass))
  {
    VkSurfaceCapabilitiesKHR surface_capabilities;
    THROW_ON_ERROR(vulkan->vkGetPhysicalDeviceSurfaceCapabilities(this->device->physical_device.device,
      surface,
      &surface_capabilities));

    this->extent2d = surface_capabilities.currentExtent;
    VkExtent3D extent3d = { this->extent2d.width, this->extent2d.height, 1 };

    std::vector<VkImageMemoryBarrier> memory_barriers;
    {
      VkImageSubresourceRange subresource_range{
        VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
      };

      VkComponentMapping component_mapping{
        VK_COMPONENT_SWIZZLE_R,
        VK_COMPONENT_SWIZZLE_G,
        VK_COMPONENT_SWIZZLE_B,
        VK_COMPONENT_SWIZZLE_A
      };

      this->color_buffer = std::make_shared<VulkanImage>(
        this->device,
        VK_IMAGE_TYPE_2D,
        surface_format.format,
        extent3d,
        subresource_range.levelCount,
        subresource_range.layerCount,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE);

      this->color_buffer_object = std::make_unique<ImageObject>(
        this->color_buffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      const auto memory = std::make_shared<VulkanMemory>(
        this->device,
        this->color_buffer_object->memory_requirements.size,
        this->color_buffer_object->memory_type_index);

      const VkDeviceSize offset = 0;
      this->color_buffer_object->bind(memory, offset);

      this->color_buffer_view = std::make_unique<VulkanImageView>(
        this->device,
        this->color_buffer->image,
        surface_format.format,
        VK_IMAGE_VIEW_TYPE_2D,
        component_mapping,
        subresource_range);

      memory_barriers.push_back({
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        0,                                                             // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                      // newLayout
        this->device->default_queue_index,                             // srcQueueFamilyIndex
        this->device->default_queue_index,                             // dstQueueFamilyIndex
        this->color_buffer->image,                                     // image
        subresource_range                                              // subresourceRange
        });
    }
    {
      VkImageSubresourceRange subresource_range{
        VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1
      };

      VkComponentMapping component_mapping{
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
      };

      this->depth_buffer = std::make_shared<VulkanImage>(
        this->device,
        VK_IMAGE_TYPE_2D,
        depth_format,
        extent3d,
        subresource_range.levelCount,
        subresource_range.layerCount,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_SHARING_MODE_EXCLUSIVE);

      this->depth_buffer_object = std::make_unique<ImageObject>(
        this->depth_buffer,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      const auto memory = std::make_shared<VulkanMemory>(
        this->device,
        this->depth_buffer_object->memory_requirements.size,
        this->depth_buffer_object->memory_type_index);

      const VkDeviceSize offset = 0;
      this->depth_buffer_object->bind(memory, offset);

      this->depth_buffer_view = std::make_unique<VulkanImageView>(
        this->device,
        this->depth_buffer->image,
        depth_format,
        VK_IMAGE_VIEW_TYPE_2D,
        component_mapping,
        subresource_range);

      memory_barriers.push_back({
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        0,                                                             // srcAccessMask
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,                  // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,              // newLayout
        this->device->default_queue_index,                             // srcQueueFamilyIndex
        this->device->default_queue_index,                             // dstQueueFamilyIndex
        this->depth_buffer->image,                                     // image
        subresource_range                                              // subresourceRange
        });
    }

    VulkanCommandBuffers command(this->device);
    {
      VulkanCommandBufferScope command_scope(command.buffer());

      vkCmdPipelineBarrier(command.buffer(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(memory_barriers.size()),
        memory_barriers.data());
    }

    command.submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    THROW_ON_ERROR(vkQueueWaitIdle(this->device->default_queue));

    std::vector<VkImageView> framebuffer_attachments{
      this->color_buffer_view->view,
      this->depth_buffer_view->view,
    };

    this->framebuffer = std::make_unique<VulkanFramebuffer>(this->device,
      this->renderpass,
      framebuffer_attachments,
      this->extent2d,
      1);
  }

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanImage> color_buffer;
  std::unique_ptr<ImageObject> color_buffer_object;
  std::unique_ptr<VulkanImageView> color_buffer_view;
  std::shared_ptr<VulkanImage> depth_buffer;
  std::unique_ptr<ImageObject> depth_buffer_object;
  std::unique_ptr<VulkanImageView> depth_buffer_view;

  std::unique_ptr<VulkanFramebuffer> framebuffer;

  VkExtent2D extent2d{ 0, 0 };
};


class VulkanSwapchainObject {
public:
  VulkanSwapchainObject(std::shared_ptr<VulkanInstance> vulkan,
                        std::shared_ptr<VulkanDevice> device,
                        std::shared_ptr<VulkanImage> src_image,
                        VkSurfaceKHR surface,
                        VkPresentModeKHR present_mode,
                        VkSurfaceFormatKHR surface_format,
                        VkSwapchainKHR oldSwapchain) :
    vulkan(std::move(vulkan)),
    device(std::move(device))
  {
    this->semaphore = std::make_unique<VulkanSemaphore>(this->device);
    
    VkSurfaceCapabilitiesKHR surface_capabilities;
    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(this->device->physical_device.device,
                                                                        surface,
                                                                        &surface_capabilities));

    this->extent2d = surface_capabilities.currentExtent;
    VkExtent3D extent3d = { this->extent2d.width, this->extent2d.height, 1 };

    this->swapchain = std::make_unique<VulkanSwapchain>(
      this->device,
      this->vulkan,
      surface,
      3,
      surface_format.format,
      surface_format.colorSpace,
      this->extent2d,
      1,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>{ this->device->default_queue_index },
      surface_capabilities.currentTransform,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      present_mode,
      VK_FALSE,
      oldSwapchain);

    auto swapchain_images = this->swapchain->getSwapchainImages();
    
    std::vector<VkImageMemoryBarrier> memory_barriers;
    for (auto& swapchain_image : swapchain_images) {
      memory_barriers.push_back({
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        0,                                                             // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // newLayout
        this->device->default_queue_index,                             // srcQueueFamilyIndex
        this->device->default_queue_index,                             // dstQueueFamilyIndex
        swapchain_image,                                               // image
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }                      // subresourceRange
        });
    }

    VulkanCommandBuffers command(this->device);
    {
      VulkanCommandBufferScope command_scope(command.buffer());

      vkCmdPipelineBarrier(command.buffer(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(memory_barriers.size()),
        memory_barriers.data());
    }

    command.submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    THROW_ON_ERROR(vkQueueWaitIdle(this->device->default_queue));

    const VkImageSubresourceRange subresource_range{
      VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
    };

    std::vector<VkImageMemoryBarrier> src_image_barriers{ {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // newLayout
        this->device->default_queue_index,                             // srcQueueFamilyIndex
        this->device->default_queue_index,                             // dstQueueFamilyIndex
        nullptr,                                                       // image
        subresource_range,                                             // subresourceRange
      },{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                                   // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                      // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // newLayout
        this->device->default_queue_index,                             // srcQueueFamilyIndex
        this->device->default_queue_index,                             // dstQueueFamilyIndex
        nullptr,                                                       // image
        subresource_range,                                             // subresourceRange
      } };

    auto dst_image_barriers = src_image_barriers;
    std::swap(dst_image_barriers[0].oldLayout, dst_image_barriers[0].newLayout);
    std::swap(dst_image_barriers[1].oldLayout, dst_image_barriers[1].newLayout);
    std::swap(dst_image_barriers[0].srcAccessMask, dst_image_barriers[0].dstAccessMask);
    std::swap(dst_image_barriers[1].srcAccessMask, dst_image_barriers[1].dstAccessMask);

    this->swap_buffers_command = std::make_unique<VulkanCommandBuffers>(this->device, swapchain_images.size());

    for (uint32_t i = 0; i < swapchain_images.size(); i++) {
      src_image_barriers[1].image = src_image->image;
      dst_image_barriers[1].image = src_image->image;
      src_image_barriers[0].image = swapchain_images[i];
      dst_image_barriers[0].image = swapchain_images[i];

      VulkanCommandBufferScope command_scope(this->swap_buffers_command->buffer(i));

      vkCmdPipelineBarrier(this->swap_buffers_command->buffer(i),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(src_image_barriers.size()),
        src_image_barriers.data());

      const VkImageSubresourceLayers subresource_layers{
        subresource_range.aspectMask,     // aspectMask
        subresource_range.baseMipLevel,   // mipLevel
        subresource_range.baseArrayLayer, // baseArrayLayer
        subresource_range.layerCount      // layerCount;
      };

      const VkOffset3D offset = {
        0, 0, 0
      };

      VkImageCopy image_copy{
        subresource_layers,             // srcSubresource
        offset,                         // srcOffset
        subresource_layers,             // dstSubresource
        offset,                         // dstOffset
        extent3d                        // extent
      };

      vkCmdCopyImage(this->swap_buffers_command->buffer(i),
        src_image->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        swapchain_images[i],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &image_copy);

      vkCmdPipelineBarrier(this->swap_buffers_command->buffer(i),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(dst_image_barriers.size()),
        dst_image_barriers.data());
    }
  }

  void swapBuffers() const
  {
    auto image_index = this->swapchain->getNextImageIndex(this->semaphore);

    this->swap_buffers_command->submit(this->device->default_queue,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      image_index,
      { this->semaphore->semaphore });

    VkPresentInfoKHR present_info{
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR, // sType
      nullptr,                            // pNext
      0,                                  // waitSemaphoreCount
      nullptr,                            // pWaitSemaphores
      1,                                  // swapchainCount
      &this->swapchain->swapchain,        // pSwapchains
      &image_index,                       // pImageIndices
      nullptr                             // pResults
    };

    THROW_ON_ERROR(this->vulkan->vkQueuePresent(this->device->default_queue, &present_info));
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanSemaphore> semaphore;
  std::unique_ptr<VulkanCommandBuffers> swap_buffers_command;
  std::unique_ptr<VulkanSwapchain> swapchain;

  VkExtent2D extent2d{ 0, 0 };
};

class VulkanViewer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanViewer)
  VulkanViewer() = delete;

  VulkanViewer(std::shared_ptr<VulkanInstance> vulkan, 
               std::shared_ptr<VulkanDevice> device,
               std::shared_ptr<::VulkanSurface> surface, 
               std::shared_ptr<Camera> camera) :
    vulkan(std::move(vulkan)),
    device(std::move(device)),
    surface(std::move(surface)),
    camera(std::move(camera))
  {
    auto physical_device = this->device->physical_device.device;
    std::vector<VkSurfaceFormatKHR> surface_formats = this->vulkan->getPhysicalDeviceSurfaceFormats(physical_device, this->surface->surface);
    this->surface_format = surface_formats[0];
    std::vector<VkPresentModeKHR> present_modes = this->vulkan->getPhysicalDeviceSurfacePresentModes(physical_device, this->surface->surface);

    this->present_mode = VK_PRESENT_MODE_FIFO_KHR;

    if (std::find(present_modes.begin(), present_modes.end(), this->present_mode) == present_modes.end()) {
      throw std::runtime_error("surface does not support VK_PRESENT_MODE_FIFO_KHR");
    }

    std::vector<VkAttachmentDescription> attachment_descriptions{ {
        0,                                                    // flags
        this->surface_format.format,                          // format
        VK_SAMPLE_COUNT_1_BIT,                                // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                         // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                     // stencilStoreOp
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,             // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL              // finalLayout
      },{
        0,                                                    // flags
        this->depth_format,                                   // format
        VK_SAMPLE_COUNT_1_BIT,                                // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                         // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                     // stencilStoreOp
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,     // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL      // finalLayout
    } };

    VkAttachmentReference depth_stencil_attachment{
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::vector<VkAttachmentReference> color_attachments{
      { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };

    std::vector<VkSubpassDescription> subpass_descriptions{ {
        0,                                                      // flags
        VK_PIPELINE_BIND_POINT_GRAPHICS,                        // pipelineBindPoint
        0,                                                      // inputAttachmentCount
        nullptr,                                                // pInputAttachments
        static_cast<uint32_t>(color_attachments.size()),        // colorAttachmentCount
        color_attachments.data(),                               // pColorAttachments
        nullptr,                                                // pResolveAttachments
        &depth_stencil_attachment,                              // pDepthStencilAttachment
        0,                                                      // preserveAttachmentCount
        nullptr                                                 // pPreserveAttachments
    } };

    this->renderpass = std::make_unique<VulkanRenderpass>(
      this->device,
      attachment_descriptions,
      subpass_descriptions);

    this->rendermanager = std::make_unique<RenderManager>(this->device);

    this->framebuffer = std::make_unique<VulkanFramebufferObject>(this->vulkan,
                                                                  this->device,
                                                                  this->renderpass,
                                                                  this->surface->surface,
                                                                  this->depth_format,
                                                                  this->surface_format);

    this->swapchain = std::make_unique<VulkanSwapchainObject>(this->vulkan,
                                                              this->device,
                                                              this->framebuffer->color_buffer,
                                                              this->surface->surface,
                                                              this->present_mode,
                                                              this->surface_format,
                                                              nullptr);

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

  void redraw() const
  {
    this->rendermanager->render(this->root.get(),
                                this->framebuffer->framebuffer->framebuffer,
                                this->renderpass->renderpass,
                                this->camera.get(),
                                this->swapchain->extent2d);

    this->swapBuffers();
  }

  void setSceneGraph(std::shared_ptr<Separator> scene)
  {
    this->root = std::move(scene);

    this->rendermanager->alloc(this->root.get());
    this->rendermanager->stage(this->root.get());

    this->rendermanager->pipeline(this->root.get(), 
                                 this->renderpass->renderpass);

    this->rendermanager->record(this->root.get(), 
                                this->framebuffer->framebuffer->framebuffer,
                                this->renderpass->renderpass, 
                                this->swapchain->extent2d);
  }

  void swapBuffers() const
  {
    try {
      this->swapchain->swapBuffers();
    }
    catch (VkErrorOutOfDateException &) {
    }
    catch (std::exception & e) {
      std::cout << e.what() << std::endl;
    }
  }

  void resize()
  {
    // make sure all work submitted is done before we start recreating stuff
    THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));

    this->framebuffer = std::make_unique<VulkanFramebufferObject>(this->vulkan,
                                                                  this->device,
                                                                  this->renderpass,
                                                                  this->surface->surface,
                                                                  this->depth_format,
                                                                  this->surface_format);

    this->swapchain = std::make_unique<VulkanSwapchainObject>(this->vulkan,
                                                              this->device,
                                                              this->framebuffer->color_buffer,
                                                              this->surface->surface,
                                                              this->present_mode,
                                                              this->surface_format,
                                                              this->swapchain->swapchain->swapchain);

    this->camera->aspectratio = static_cast<float>(this->swapchain->extent2d.width) /
                                static_cast<float>(this->swapchain->extent2d.height);

    this->rendermanager->record(this->root.get(),
                                this->framebuffer->framebuffer->framebuffer,
                                this->renderpass->renderpass,
                                this->swapchain->extent2d);

    this->redraw();
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<::VulkanSurface> surface;
  VkPresentModeKHR present_mode{ VK_PRESENT_MODE_FIFO_KHR }; // always present?
  VkSurfaceFormatKHR surface_format{};
  std::shared_ptr<Camera> camera;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanFramebufferObject> framebuffer;
  std::unique_ptr<RenderManager> rendermanager;
  std::unique_ptr<VulkanSwapchainObject> swapchain;
  std::shared_ptr<Separator> root;

  VkFormat depth_format{ VK_FORMAT_D32_SFLOAT };
};
