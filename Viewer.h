#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/VulkanObjects.h>
#include <Innovator/Actions.h>
#include <Innovator/Nodes.h>

#include <map>
#include <string>
#include <memory>
#include <utility>
#include <vector>
#include <iostream>

class VulkanViewer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanViewer);
  VulkanViewer() = delete;

  VulkanViewer(std::shared_ptr<VulkanInstance> vulkan, VkSurfaceKHR surface)
    : vulkan(std::move(vulkan)), 
      surface(surface)
  {
    VkPhysicalDeviceFeatures required_device_features;
    ::memset(&required_device_features, VK_FALSE, sizeof(VkPhysicalDeviceFeatures));
    required_device_features.geometryShader = VK_TRUE;
    required_device_features.tessellationShader = VK_TRUE;
    required_device_features.textureCompressionBC = VK_TRUE;

    VulkanPhysicalDevice physical_device = this->vulkan->selectPhysicalDevice(required_device_features);

    std::vector<VkBool32> presentation_filter(physical_device.queue_family_properties.size());
    for (uint32_t i = 0; i < physical_device.queue_family_properties.size(); i++) {
      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device.device, i, this->surface, &presentation_filter[i]);
    }

    float queue_priorities[1] = { 1.0f };
    uint32_t queue_index = physical_device.getQueueIndex(
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
      presentation_filter);

    std::vector<VkDeviceQueueCreateInfo> queue_create_info{ {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,   // sType
        nullptr,                                      // pNext
        0,                                            // flags
        queue_index,                                  // queueFamilyIndex
        1,                                            // queueCount   
        queue_priorities                              // pQueuePriorities
      } };

    std::vector<const char *> device_layers{
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };
    std::vector<const char *> device_extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    this->device = std::make_shared<VulkanDevice>(
      physical_device,
      required_device_features,
      device_layers,
      device_extensions,
      queue_create_info);

    this->semaphore = std::make_unique<VulkanSemaphore>(this->device);

    uint32_t format_count;
    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceFormats(physical_device.device, this->surface, &format_count, nullptr));
    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceFormats(physical_device.device, this->surface, &format_count, surface_formats.data()));

    this->surface_format = surface_formats[0];

    this->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;

    uint32_t mode_count;
    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfacePresentModes(physical_device.device, this->surface, &mode_count, nullptr));
    std::vector<VkPresentModeKHR> present_modes(mode_count);
    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfacePresentModes(physical_device.device, this->surface, &mode_count, present_modes.data()));

    if (std::find(present_modes.begin(), present_modes.end(), present_mode) == present_modes.end()) {
      throw std::runtime_error("surface does not support VK_PRESENT_MODE_MAILBOX_KHR");
    }
    this->handleeventaction = std::make_unique<HandleEventAction>();

    this->depth_format = VK_FORMAT_D32_SFLOAT;
    this->color_format = this->surface_format.format;

    std::vector<VkAttachmentDescription> attachment_descriptions{ {
        0,                                                    // flags
        this->color_format,                                   // format
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

    this->resize();
  }

  ~VulkanViewer()
  {
    try {
      // make sure all work submitted to GPU is done before we start deleting stuff...
      THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void setSceneGraph(std::shared_ptr<class Node> scene)
  {
    this->renderaction->clearCache();

    this->root = std::make_shared<Separator>();
    this->camera = SearchAction<Camera>(scene);

    if (!this->camera) {
      this->camera = std::make_shared<Camera>();
      this->root->children = { this->camera, scene };
      this->camera->viewAll(this->root);
    }
    else {
      this->root->children = { scene };
    }
  }

  void render()
  {
    try {
      this->renderaction->apply(this->root);

      uint32_t image_index = this->swapchain->getNextImageIndex(this->semaphore);

      this->swap_buffers_command->submit(this->device->default_queue,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        image_index,
        { this->semaphore->semaphore });

      VkPresentInfoKHR present_info = {
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
    catch (VkErrorOutOfDateException &) {
      this->resize();
    }
    catch (std::exception & e) {
      std::cout << e.what() << std::endl;
    }
  }

  void resize()
  {
    // make sure all work submitted is done before we start recreating stuff
    THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));

    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(
      this->device->physical_device.device,
      this->surface,
      &this->surface_capabilities));

    VkExtent2D extent2d = this->surface_capabilities.currentExtent;
    VkExtent3D extent3d = { extent2d.width, extent2d.height, 1 };

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

      this->color_buffer = std::make_unique<ImageObject>(
        this->device,
        this->color_format,
        extent3d,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        subresource_range,
        component_mapping);

      memory_barriers.push_back({
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        0,                                                             // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                      // newLayout
        this->device->default_queue_index,                             // srcQueueFamilyIndex
        this->device->default_queue_index,                             // dstQueueFamilyIndex
        this->color_buffer->image->image,                              // image
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

      this->depth_buffer = std::make_unique<ImageObject>(
        this->device,
        this->depth_format,
        extent3d,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        subresource_range,
        component_mapping);

      memory_barriers.push_back({
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        0,                                                             // srcAccessMask
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,                  // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,              // newLayout
        this->device->default_queue_index,                             // srcQueueFamilyIndex
        this->device->default_queue_index,                             // dstQueueFamilyIndex
        this->depth_buffer->image->image,                              // image
        subresource_range                                              // subresourceRange
        });
    }

    std::vector<VkImageView> framebuffer_attachments{
      this->color_buffer->view->view,
      this->depth_buffer->view->view
    };

    this->framebuffer = std::make_unique<VulkanFramebuffer>(
      this->device,
      this->renderpass,
      framebuffer_attachments,
      extent2d,
      1);

    this->renderaction = std::make_unique<RenderAction>(
      this->device,
      this->renderpass,
      this->framebuffer,
      extent2d);

    this->swapchain = std::make_unique<VulkanSwapchain>(
      this->device,
      this->vulkan,
      this->surface,
      3,
      this->surface_format.format,
      this->surface_format.colorSpace,
      extent2d,
      1,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>(this->device->default_queue_index),
      this->surface_capabilities.currentTransform,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      this->present_mode,
      VK_FALSE,
      this->swapchain ? this->swapchain->swapchain : nullptr);

    std::vector<VkImage> swapchain_images = this->swapchain->getSwapchainImages();
    {
      for (VkImage & swapchain_image : swapchain_images) {
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
      command.begin();
      vkCmdPipelineBarrier(
        command.buffer(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(memory_barriers.size()),
        memory_barriers.data());

      command.end();
      command.submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      THROW_ON_ERROR(vkQueueWaitIdle(this->device->default_queue));
    }

    VkImageSubresourceRange subresource_range{
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

    std::vector<VkImageMemoryBarrier> dst_image_barriers = src_image_barriers;
    std::swap(dst_image_barriers[0].oldLayout, dst_image_barriers[0].newLayout);
    std::swap(dst_image_barriers[1].oldLayout, dst_image_barriers[1].newLayout);
    std::swap(dst_image_barriers[0].srcAccessMask, dst_image_barriers[0].dstAccessMask);
    std::swap(dst_image_barriers[1].srcAccessMask, dst_image_barriers[1].dstAccessMask);

    this->swap_buffers_command = std::make_unique<VulkanCommandBuffers>(this->device, swapchain_images.size());

    for (uint32_t i = 0; i < swapchain_images.size(); i++) {
      src_image_barriers[1].image = color_buffer->image->image;
      dst_image_barriers[1].image = color_buffer->image->image;
      src_image_barriers[0].image = swapchain_images[i];
      dst_image_barriers[0].image = swapchain_images[i];

      this->swap_buffers_command->begin(i);

      vkCmdPipelineBarrier(this->swap_buffers_command->buffer(i),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(src_image_barriers.size()),
        src_image_barriers.data());

      VkImageSubresourceLayers subresource_layers{
        subresource_range.aspectMask,     // aspectMask
        subresource_range.baseMipLevel,   // mipLevel
        subresource_range.baseArrayLayer, // baseArrayLayer
        subresource_range.layerCount      // layerCount;
      };

      VkOffset3D offset = {
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
        this->color_buffer->image->image,
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

      this->swap_buffers_command->end(i);
    }
  }

  void keyDown(uint32_t key)
  {
    std::map<uint32_t, std::string> keymap{
      { 0x25, "LEFT_ARROW" },{ 0x26, "UP_ARROW" },{ 0x27, "RIGHT_ARROW" },{ 0x28, "DOWN_ARROW" },
    { 0x30, "0" },{ 0x31, "1" },{ 0x32, "2" },{ 0x33, "3" },{ 0x34, "4" },{ 0x35, "5" },{ 0x36, "6" },{ 0x37, "7" },{ 0x38, "8" },{ 0x39, "9" },
    { 0x41, "A" },{ 0x42, "B" },{ 0x43, "C" },{ 0x44, "D" },{ 0x45, "E" },{ 0x46, "F" },{ 0x47, "G" },{ 0x48, "H" },{ 0x49, "I" },{ 0x4A, "J" },
    { 0x4B, "K" },{ 0x4C, "L" },{ 0x4D, "M" },{ 0x4E, "N" },{ 0x4F, "O" },{ 0x50, "P" },{ 0x51, "Q" },{ 0x52, "R" },{ 0x53, "S" },{ 0x54, "T" },
    { 0x55, "U" },{ 0x56, "V" },{ 0x57, "W" },{ 0x58, "X" },{ 0x59, "Y" },{ 0x5A, "Z" },
    };
    this->handleeventaction->key = keymap[key];
    this->handleeventaction->apply(this->root);
  }

  void mousePressed(uint32_t x, uint32_t y, int button)
  {
    this->button = button;
    this->mouse_pos = glm::vec2(x, y);
    this->mouse_pressed = true;
  }

  void mouseReleased(uint32_t x, uint32_t y, int button)
  {
    this->mouse_pressed = false;
  }

  void mouseMoved(uint32_t x, uint32_t y)
  {
    if (this->mouse_pressed) {
      glm::vec2 pos = glm::vec2(x, y);
      glm::vec2 dx = this->mouse_pos - pos;
      switch (this->button) {
      case 2: this->camera->pan(dx * .01f); break;
      case 0: this->camera->orbit(dx * .01f); break;
      case 1: this->camera->zoom(dx[1] * .01f); break;
      default: break;
      }
      this->mouse_pos = pos;
    }
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanSemaphore> semaphore;
  std::unique_ptr<VulkanCommandBuffers> swap_buffers_command;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanFramebuffer> framebuffer;
  std::unique_ptr<ImageObject> color_buffer;
  std::unique_ptr<ImageObject> depth_buffer;
  std::unique_ptr<RenderAction> renderaction;
  std::unique_ptr<HandleEventAction> handleeventaction;
  std::unique_ptr<VulkanSwapchain> swapchain;
  std::shared_ptr<Separator> root;
  std::shared_ptr<Camera> camera;

  VkFormat depth_format;
  VkFormat color_format;

  VkSurfaceKHR surface;
  VkPresentModeKHR present_mode;
  VkSurfaceFormatKHR surface_format;
  VkSurfaceCapabilitiesKHR surface_capabilities;

  int button;
  bool mouse_pressed;
  glm::vec2 mouse_pos;
};
