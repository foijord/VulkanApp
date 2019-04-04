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

class FramebufferAttachment : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(FramebufferAttachment)
  virtual ~FramebufferAttachment() = default;

  FramebufferAttachment(VkFormat format,
                        VkImageUsageFlags usage,
                        VkAccessFlags dstAccessMask,
                        VkImageLayout newLayout,
                        VkImageAspectFlags aspectMask)
  {
    VkAccessFlags srcAccessMask = 0;
    VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageSubresourceRange subresource_range{
      aspectMask, 0, 1, 0, 1
    };

    this->image = std::make_shared<Image>(
      VK_IMAGE_TYPE_2D,
      format,
      subresource_range.levelCount,
      subresource_range.layerCount,
      VK_SAMPLE_COUNT_1_BIT,
      VK_IMAGE_TILING_OPTIMAL,
      usage,
      VK_SHARING_MODE_EXCLUSIVE,
      0);

    auto memory_allocator = std::make_shared<ImageMemoryAllocator>(
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkComponentMapping component_mapping{
      VK_COMPONENT_SWIZZLE_R,
      VK_COMPONENT_SWIZZLE_G,
      VK_COMPONENT_SWIZZLE_B,
      VK_COMPONENT_SWIZZLE_A
    };

    this->image_view = std::make_shared<ImageView>(
      format,
      VK_IMAGE_VIEW_TYPE_2D,
      component_mapping,
      subresource_range);

    auto memory_barrier = std::make_shared<ImageMemoryBarrier>(
      srcAccessMask,
      dstAccessMask,
      oldLayout,
      newLayout,
      subresource_range);

    auto pipeline_barrier = std::make_shared<PipelineBarrier>(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                              VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                              0);

    this->children = {
      this->image,
      memory_allocator,
      this->image_view,
      memory_barrier,
      pipeline_barrier
    };
  }

private:
  void doStage(MemoryStager * stager) override
  {
    Group::doStage(stager);
    stager->state.framebuffer_attachments.push_back(this->image_view->imageview->view);
  }

public:
  std::shared_ptr<Image> image;
  std::shared_ptr<ImageView> image_view;
};

class VulkanFramebufferObject : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanFramebufferObject)
  virtual ~VulkanFramebufferObject() = default;
  VulkanFramebufferObject() = default;

  std::unique_ptr<VulkanFramebuffer> framebuffer;

private:
  void doStage(MemoryStager * stager) override
  {
    StateScope<MemoryStager, StageState> scope(stager);
    Group::doStage(stager);

    this->framebuffer = std::make_unique<VulkanFramebuffer>(stager->device,
                                                            stager->renderpass,
                                                            stager->state.framebuffer_attachments,
                                                            stager->extent,
                                                            1);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.framebuffer = this->framebuffer->framebuffer;
  }

  void doRender(SceneRenderer * renderer) override
  {
    renderer->state.framebuffer = this->framebuffer->framebuffer;
  }
};

class VulkanSwapchainObject : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSwapchainObject)
  virtual ~VulkanSwapchainObject() = default;

  VulkanSwapchainObject(std::shared_ptr<Image> src_image,
                        VkSurfaceKHR surface,
                        VkPresentModeKHR present_mode,
                        VkSurfaceFormatKHR surface_format) :
      src_image(std::move(src_image)),
      surface(surface),
      present_mode(present_mode),
      surface_format(surface_format)
  {}

private:
  void doStage(MemoryStager * stager) override
  {
    VkExtent3D extent3d = { stager->extent.width, stager->extent.height, 1 };

    VkSwapchainKHR oldSwapchain = (this->swapchain) ? this->swapchain->swapchain : nullptr;

    this->swapchain = std::make_unique<VulkanSwapchain>(
      stager->device,
      stager->vulkan,
      surface,
      3,
      surface_format.format,
      surface_format.colorSpace,
      stager->extent,
      1,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>{ stager->device->default_queue_index, },
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
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
        stager->device->default_queue_index,                           // srcQueueFamilyIndex
        stager->device->default_queue_index,                           // dstQueueFamilyIndex
        swapchain_image,                                               // image
        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }                      // subresourceRange
        });
    }

    VulkanCommandBuffers command(stager->device);
    {
      VulkanCommandBufferScope command_scope(command.buffer());

      vkCmdPipelineBarrier(command.buffer(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(memory_barriers.size()),
        memory_barriers.data());
    }

    command.submit(stager->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    THROW_ON_ERROR(vkQueueWaitIdle(stager->device->default_queue));

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
        stager->device->default_queue_index,                           // srcQueueFamilyIndex
        stager->device->default_queue_index,                           // dstQueueFamilyIndex
        nullptr,                                                       // image
        subresource_range,                                             // subresourceRange
      },{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                                   // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                      // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // newLayout
        stager->device->default_queue_index,                           // srcQueueFamilyIndex
        stager->device->default_queue_index,                           // dstQueueFamilyIndex
        nullptr,                                                       // image
        subresource_range,                                             // subresourceRange
      } };

    auto dst_image_barriers = src_image_barriers;
    std::swap(dst_image_barriers[0].oldLayout, dst_image_barriers[0].newLayout);
    std::swap(dst_image_barriers[1].oldLayout, dst_image_barriers[1].newLayout);
    std::swap(dst_image_barriers[0].srcAccessMask, dst_image_barriers[0].dstAccessMask);
    std::swap(dst_image_barriers[1].srcAccessMask, dst_image_barriers[1].dstAccessMask);

    this->swap_buffers_command = std::make_unique<VulkanCommandBuffers>(stager->device, swapchain_images.size());

    for (uint32_t i = 0; i < swapchain_images.size(); i++) {
      src_image_barriers[1].image = src_image->image->image;
      dst_image_barriers[1].image = src_image->image->image;
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
        src_image->image->image,
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

  void doRender(SceneRenderer * renderer) override
  {
    VulkanSemaphore semaphore(renderer->device);
    auto image_index = this->swapchain->getNextImageIndex(semaphore.semaphore);

    this->swap_buffers_command->submit(renderer->device->default_queue,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      image_index,
      { semaphore.semaphore });

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

    THROW_ON_ERROR(renderer->vulkan->vkQueuePresent(renderer->device->default_queue, &present_info));
  }

  std::shared_ptr<Image> src_image;
  VkSurfaceKHR surface;
  VkPresentModeKHR present_mode;
  VkSurfaceFormatKHR surface_format;

  std::unique_ptr<VulkanCommandBuffers> swap_buffers_command;
  std::unique_ptr<VulkanSwapchain> swapchain;
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

    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(this->device->physical_device.device,
                                                                        this->surface->surface,
                                                                        &this->surface_capabilities));

    this->rendermanager = std::make_unique<RenderManager>(this->vulkan, this->device, this->renderpass);
    this->rendermanager->resize(this->surface_capabilities.currentExtent);

    auto colorbuffer = std::make_shared<FramebufferAttachment>(this->surface_format.format,
                                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                                               VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                                               VK_IMAGE_ASPECT_COLOR_BIT);

    auto depthbuffer = std::make_shared<FramebufferAttachment>(this->depth_format,
                                                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                                               VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                               VK_IMAGE_ASPECT_DEPTH_BIT);

    this->framebuffer = std::make_unique<VulkanFramebufferObject>();

    this->framebuffer->children = { 
      colorbuffer,
      depthbuffer 
    };

    this->swapchain = std::make_unique<VulkanSwapchainObject>(colorbuffer->image,
                                                              this->surface->surface,
                                                              this->present_mode,
                                                              this->surface_format);
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
                                this->camera.get());
  }

  void setSceneGraph(std::shared_ptr<Separator> scene)
  {
    this->root = std::make_shared<Separator>();

    this->root->children = {
      this->framebuffer,
      std::move(scene),
      this->swapchain
    };

    this->rendermanager->alloc(this->root.get());
    this->rendermanager->stage(this->root.get());
    this->rendermanager->pipeline(this->root.get());
    this->rendermanager->record(this->root.get());
  }

  void resize()
  {
    // make sure all work submitted is done before we start recreating stuff
    THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));

    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(this->device->physical_device.device,
                                                                        this->surface->surface,
                                                                        &this->surface_capabilities));

    this->rendermanager->resize(this->surface_capabilities.currentExtent);

    this->rendermanager->alloc(this->framebuffer.get());
    this->rendermanager->stage(this->framebuffer.get());

    this->rendermanager->alloc(this->swapchain.get());
    this->rendermanager->stage(this->swapchain.get());
    
    this->rendermanager->record(this->root.get());

    this->camera->aspectratio = static_cast<float>(this->surface_capabilities.currentExtent.width) /
                                static_cast<float>(this->surface_capabilities.currentExtent.height);

    this->redraw();
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<::VulkanSurface> surface;
  VkPresentModeKHR present_mode{ VK_PRESENT_MODE_FIFO_KHR }; // always present?
  VkSurfaceFormatKHR surface_format{};
  VkSurfaceCapabilitiesKHR surface_capabilities{};
  std::shared_ptr<Camera> camera;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanFramebufferObject> framebuffer;
  std::unique_ptr<RenderManager> rendermanager;
  std::shared_ptr<VulkanSwapchainObject> swapchain;
  std::shared_ptr<Separator> root;

  VkFormat depth_format{ VK_FORMAT_D32_SFLOAT };
};
