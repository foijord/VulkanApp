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

class FramebufferAttachment : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(FramebufferAttachment)
  virtual ~FramebufferAttachment() = default;

  FramebufferAttachment(VkFormat format,
                        VkImageUsageFlags usage,
                        VkImageAspectFlags aspectMask) :
    format(format),
    usage(usage),
    aspectMask(aspectMask)
  {}

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    VkExtent3D extent = { allocator->extent.width, allocator->extent.height, 1 };
    this->image = std::make_shared<VulkanImage>(allocator->device,
                                                VK_IMAGE_TYPE_2D,
                                                this->format,
                                                extent,
                                                this->subresource_range.levelCount,
                                                this->subresource_range.layerCount,
                                                VK_SAMPLE_COUNT_1_BIT,
                                                VK_IMAGE_TILING_OPTIMAL,
                                                this->usage,
                                                VK_SHARING_MODE_EXCLUSIVE,
                                                0);

    this->imageobject = std::make_shared<ImageObject>(allocator->device,
                                                      this->image,
                                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    allocator->imageobjects.push_back(this->imageobject);

    allocator->alloc_callbacks.push_back([this](MemoryAllocator * allocator) {
      this->imageview = std::make_shared<VulkanImageView>(allocator->device,
                                                          this->image->image,
                                                          this->format,
                                                          VK_IMAGE_VIEW_TYPE_2D,
                                                          this->component_mapping,
                                                          this->subresource_range);

      allocator->state.framebuffer_attachments.push_back(this->imageview->view);
    });
  }

  VkFormat format;
  VkImageUsageFlags usage;
  VkImageAspectFlags aspectMask;
 
  VkImageSubresourceRange subresource_range{
    aspectMask, 0, 1, 0, 1
  };

  VkComponentMapping component_mapping{
    VK_COMPONENT_SWIZZLE_R,
    VK_COMPONENT_SWIZZLE_G,
    VK_COMPONENT_SWIZZLE_B,
    VK_COMPONENT_SWIZZLE_A
  };

public:
  std::shared_ptr<VulkanImage> image;
  std::shared_ptr<ImageObject> imageobject;
  std::shared_ptr<VulkanImageView> imageview;
};

class SubpassObject {
public:
  NO_COPY_OR_ASSIGNMENT(SubpassObject)

  SubpassObject(VkSubpassDescriptionFlags flags,
                VkPipelineBindPoint bind_point,
                std::vector<VkAttachmentReference> input_attachments,
                std::vector<VkAttachmentReference> color_attachments,
                std::vector<VkAttachmentReference> resolve_attachments,
                VkAttachmentReference depth_stencil_attachment,
                std::vector<uint32_t> preserve_attachments) :
    input_attachments(input_attachments),
    color_attachments(color_attachments),
    resolve_attachments(resolve_attachments),
    depth_stencil_attachment(depth_stencil_attachment),
    preserve_attachments(preserve_attachments)
  {
    this->description = {
      flags,                                                    // flags
      bind_point,                                               // pipelineBindPoint
      static_cast<uint32_t>(this->input_attachments.size()),    // inputAttachmentCount
      this->input_attachments.data(),                           // pInputAttachments
      static_cast<uint32_t>(this->color_attachments.size()),    // colorAttachmentCount
      this->color_attachments.data(),                           // pColorAttachments
      this->resolve_attachments.data(),                         // pResolveAttachments
      &this->depth_stencil_attachment,                          // pDepthStencilAttachment
      static_cast<uint32_t>(this->preserve_attachments.size()), // preserveAttachmentCount
      this->preserve_attachments.data()                         // pPreserveAttachments
    };
  }

  VkSubpassDescription description;

private:
  std::vector<VkAttachmentReference> input_attachments;
  std::vector<VkAttachmentReference> color_attachments;
  std::vector<VkAttachmentReference> resolve_attachments;
  VkAttachmentReference depth_stencil_attachment;
  std::vector<uint32_t> preserve_attachments;
};

class FramebufferObject : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(FramebufferObject)
  FramebufferObject() = delete;
  virtual ~FramebufferObject() = default;

  FramebufferObject(std::vector<std::shared_ptr<Node>> framebuffer_attachments)
  {
    this->children = framebuffer_attachments;
  }

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    Group::doAlloc(allocator);
    allocator->alloc_callbacks.push_back([this](MemoryAllocator * allocator) {
      this->framebuffer = std::make_unique<VulkanFramebuffer>(allocator->device,
                                                              allocator->state.renderpass,
                                                              allocator->state.framebuffer_attachments,
                                                              allocator->extent,
                                                              1);
    });
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.framebuffer = this->framebuffer->framebuffer;
  }


public:
  std::unique_ptr<VulkanFramebuffer> framebuffer;
};

class RenderpassObject : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(RenderpassObject)
  virtual ~RenderpassObject() = default;

  RenderpassObject(std::vector<VkAttachmentDescription> attachments,
                   std::vector<std::shared_ptr<SubpassObject>> subpasses,
                   std::vector<std::shared_ptr<Node>> framebuffer_attachments) :
    attachments(std::move(attachments)),
    subpasses(std::move(subpasses))
  {
    for (auto & subpass : this->subpasses) {
      this->subpass_descriptions.push_back(subpass->description);
    }

    this->framebuffer = std::make_shared<FramebufferObject>(framebuffer_attachments);
  }

  void resize(std::shared_ptr<VulkanDevice> device, VkExtent2D extent)
  {
    MemoryAllocator allocator(device, extent);
    allocator.state.renderpass = this->renderpass;
    this->framebuffer->alloc(&allocator);
  }

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    this->renderpass = std::make_shared<VulkanRenderpass>(allocator->device,
                                                          this->attachments,
                                                          this->subpass_descriptions);

    allocator->state.renderpass = this->renderpass;
    this->framebuffer->alloc(allocator);
    
    Group::doAlloc(allocator);
  }

  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.renderpass = this->renderpass;
    Group::doPipeline(creator);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.renderpass = this->renderpass;
    this->framebuffer->record(recorder);

    Group::doRecord(recorder);
  }

  void doRender(SceneRenderer * renderer) override
  {
    const VkRect2D renderarea{
      { 0, 0 },                 // offset
      renderer->state.extent    // extent
    };

    const std::vector<VkClearValue> clearvalues{
      { { { 0.0f, 0.0f, 0.0f, 0.0f } } },
      { { { 1.0f, 0 } } }
    };

    VulkanCommandBufferScope commandbuffer(renderer->command->buffer());

    VulkanRenderPassScope renderpass_scope(this->renderpass->renderpass,
                                           this->framebuffer->framebuffer->framebuffer,
                                           renderarea,
                                           clearvalues,
                                           renderer->command->buffer());

    Group::doRender(renderer);
  }

  std::vector<VkAttachmentDescription> attachments;
  std::vector<std::shared_ptr<SubpassObject>> subpasses;
  std::vector<VkSubpassDescription> subpass_descriptions;
  std::shared_ptr<FramebufferObject> framebuffer;
  std::shared_ptr<VulkanRenderpass> renderpass;
};

class SwapchainObject : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(SwapchainObject)
  virtual ~SwapchainObject() = default;

  SwapchainObject(std::shared_ptr<FramebufferAttachment> color_attachment,
                  VkSurfaceKHR surface,
                  VkPresentModeKHR present_mode,
                  VkSurfaceFormatKHR surface_format) :
      color_attachment(std::move(color_attachment)),
      surface(surface),
      present_mode(present_mode),
      surface_format(surface_format)
  {}

  void present(std::shared_ptr<VulkanInstance> vulkan, std::shared_ptr<VulkanDevice> device)
  {
    THROW_ON_ERROR(vulkan->vkAcquireNextImage(device->device,
                                              this->swapchain->swapchain,
                                              UINT64_MAX,
                                              this->swapchain_image_ready->semaphore,
                                              nullptr,
                                              &this->image_index));

    std::vector<VkSemaphore> wait_semaphores = { this->swapchain_image_ready->semaphore };
    std::vector<VkSemaphore> signal_semaphores = { this->swap_buffers_finished->semaphore };

    this->swap_buffers_command->submit(device->default_queue,
                                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                       this->image_index,
                                       wait_semaphores,
                                       signal_semaphores);

    VkPresentInfoKHR present_info{
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,              // sType
      nullptr,                                         // pNext
      static_cast<uint32_t>(signal_semaphores.size()), // waitSemaphoreCount
      signal_semaphores.data(),                        // pWaitSemaphores
      1,                                               // swapchainCount
      &this->swapchain->swapchain,                     // pSwapchains
      &this->image_index,                              // pImageIndices
      nullptr                                          // pResults
    };

    THROW_ON_ERROR(vulkan->vkQueuePresent(device->default_queue, &present_info));
  }


private:
  void doStage(MemoryStager * stager) override
  {
    this->swapchain_image_ready = std::make_unique<VulkanSemaphore>(stager->device);
    this->swap_buffers_finished = std::make_unique<VulkanSemaphore>(stager->device);

    VkSwapchainKHR prevswapchain = (this->swapchain) ? this->swapchain->swapchain : nullptr;

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
      prevswapchain);

    uint32_t count;
    THROW_ON_ERROR(stager->vulkan->vkGetSwapchainImages(stager->device->device, 
                                                        this->swapchain->swapchain, 
                                                        &count, 
                                                        nullptr));
    this->swapchain_images.resize(count);
    THROW_ON_ERROR(stager->vulkan->vkGetSwapchainImages(stager->device->device, 
                                                        this->swapchain->swapchain, 
                                                        &count, 
                                                        this->swapchain_images.data()));

    std::vector<VkImageMemoryBarrier> image_barriers(count);
    for (uint32_t i = 0; i < count; i++) {
      image_barriers[i] = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType
        nullptr,                                // pNext
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        stager->device->default_queue_index,    // srcQueueFamilyIndex
        stager->device->default_queue_index,    // dstQueueFamilyIndex
        this->swapchain_images[i],              // image
        this->subresource_range
      };
    }

    vkCmdPipelineBarrier(stager->command,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr,
                         count, image_barriers.data());
  }

  void doRecord(CommandRecorder * recorder) override
  {
    this->swap_buffers_command = std::make_unique<VulkanCommandBuffers>(recorder->device, 
                                                                        this->swapchain_images.size(),
                                                                        VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    for (size_t i = 0; i < this->swapchain_images.size(); i++) {
      VkImageMemoryBarrier src_image_barriers[2] = { 
      {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // newLayout
        recorder->device->default_queue_index,                         // srcQueueFamilyIndex
        recorder->device->default_queue_index,                         // dstQueueFamilyIndex
        this->swapchain_images[i],                                     // image
        this->subresource_range,                                       // subresourceRange
      },{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                                   // dstAccessMask
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                      // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // newLayout
        recorder->device->default_queue_index,                         // srcQueueFamilyIndex
        recorder->device->default_queue_index,                         // dstQueueFamilyIndex
        this->color_attachment->image->image,                          // image
        this->subresource_range,                                       // subresourceRange
      } };

      VulkanCommandBufferScope command_scope(this->swap_buffers_command->buffer(i));

      vkCmdPipelineBarrier(this->swap_buffers_command->buffer(i),
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 0, nullptr,
                           2, src_image_barriers);

      const VkImageSubresourceLayers subresource_layers{
        this->subresource_range.aspectMask,     // aspectMask
        this->subresource_range.baseMipLevel,   // mipLevel
        this->subresource_range.baseArrayLayer, // baseArrayLayer
        this->subresource_range.layerCount      // layerCount;
      };

      const VkOffset3D offset = {
        0, 0, 0
      };

      VkExtent3D extent3d = { recorder->extent.width, recorder->extent.height, 1 };

      VkImageCopy image_copy{
        subresource_layers,             // srcSubresource
        offset,                         // srcOffset
        subresource_layers,             // dstSubresource
        offset,                         // dstOffset
        extent3d                        // extent
      };

      vkCmdCopyImage(this->swap_buffers_command->buffer(i),
                     color_attachment->image->image,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     this->swapchain_images[i],
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1, &image_copy);

      VkImageMemoryBarrier dst_image_barriers[2] = { 
      {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // newLayout
        recorder->device->default_queue_index,                         // srcQueueFamilyIndex
        recorder->device->default_queue_index,                         // dstQueueFamilyIndex
        this->swapchain_images[i],                                     // image
        this->subresource_range,                                       // subresourceRange
      },{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_TRANSFER_READ_BIT,                                   // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // oldLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,                      // newLayout
        recorder->device->default_queue_index,                         // srcQueueFamilyIndex
        recorder->device->default_queue_index,                         // dstQueueFamilyIndex
        this->color_attachment->image->image,                          // image
        this->subresource_range,                                       // subresourceRange
      } };

      vkCmdPipelineBarrier(this->swap_buffers_command->buffer(i),
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 0, nullptr,
                           2, dst_image_barriers);
    }
  }

  std::shared_ptr<FramebufferAttachment> color_attachment;
  VkSurfaceKHR surface;
  VkPresentModeKHR present_mode;
  VkSurfaceFormatKHR surface_format;

  std::unique_ptr<VulkanSwapchain> swapchain;
  std::vector<VkImage> swapchain_images;
  std::unique_ptr<VulkanCommandBuffers> swap_buffers_command;
  std::unique_ptr<VulkanSemaphore> swapchain_image_ready;
  std::unique_ptr<VulkanSemaphore> swap_buffers_finished;

  const VkImageSubresourceRange subresource_range{
    VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
  };

  uint32_t image_index;
};

class VulkanViewer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanViewer)
  VulkanViewer() = delete;

  VulkanViewer(std::shared_ptr<VulkanInstance> vulkan, 
               std::shared_ptr<VulkanDevice> device,
               std::shared_ptr<::VulkanSurface> surface, 
               std::shared_ptr<Separator> scene) :
    vulkan(std::move(vulkan)),
    device(std::move(device)),
    surface(std::move(surface))
  {
    auto physical_device = this->device->physical_device.device;
    std::vector<VkSurfaceFormatKHR> surface_formats = this->vulkan->getPhysicalDeviceSurfaceFormats(physical_device, this->surface->surface);
    VkSurfaceFormatKHR surface_format = surface_formats[0];
    VkFormat depth_format{ VK_FORMAT_D32_SFLOAT };

    std::vector<VkPresentModeKHR> present_modes = this->vulkan->getPhysicalDeviceSurfacePresentModes(physical_device, this->surface->surface);

    VkPresentModeKHR present_mode{ VK_PRESENT_MODE_FIFO_KHR }; // always present?

    if (std::find(present_modes.begin(), present_modes.end(), present_mode) == present_modes.end()) {
      throw std::runtime_error("surface does not support VK_PRESENT_MODE_FIFO_KHR");
    }


    THROW_ON_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(this->device->physical_device.device,
                                                                        this->surface->surface,
                                                                        &this->surface_capabilities));

    auto color_attachment = std::make_shared<FramebufferAttachment>(surface_format.format,
                                                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                                                    VK_IMAGE_ASPECT_COLOR_BIT);

    auto depth_attachment = std::make_shared<FramebufferAttachment>(depth_format,
                                                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                                    VK_IMAGE_ASPECT_DEPTH_BIT);

    this->renderpass = std::make_shared<RenderpassObject>(
      std::vector<VkAttachmentDescription>{
      {
        0,                                                    // flags
        surface_format.format,                                // format
        VK_SAMPLE_COUNT_1_BIT,                                // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                         // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                     // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                            // initialLayout
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL              // finalLayout
      },{
        0,                                                    // flags
        depth_format,                                         // format
        VK_SAMPLE_COUNT_1_BIT,                                // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                         // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                     // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                            // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL      // finalLayout
      } },
      std::vector<std::shared_ptr<SubpassObject>>{
      std::make_shared<SubpassObject>(
        0,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        std::vector<VkAttachmentReference>{},
        std::vector<VkAttachmentReference>{ { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } },
        std::vector<VkAttachmentReference>{},
        VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
        std::vector<uint32_t>{}
      )},
        std::vector<std::shared_ptr<Node>>{
          color_attachment,
          depth_attachment
      });

    this->swapchain = std::make_unique<SwapchainObject>(color_attachment,
                                                        this->surface->surface,
                                                        present_mode,
                                                        surface_format);

    this->rendermanager = std::make_unique<RenderManager>(this->vulkan, 
                                                          this->device);

    this->rendermanager->resize(this->surface_capabilities.currentExtent);

    this->renderpass->children = {
      scene
    };

    this->rendermanager->alloc(this->renderpass.get());
    this->rendermanager->stage(this->renderpass.get());
    this->rendermanager->pipeline(this->renderpass.get());
    this->rendermanager->record(this->renderpass.get());

    this->rendermanager->stage(this->swapchain.get());
    this->rendermanager->record(this->swapchain.get());
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
      this->rendermanager->render(this->renderpass.get());
      this->swapchain->present(this->vulkan, this->device);
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

    this->rendermanager->resize(this->surface_capabilities.currentExtent);

    this->renderpass->resize(this->device, this->surface_capabilities.currentExtent);
    this->rendermanager->record(this->renderpass.get());

    this->rendermanager->stage(this->swapchain.get());
    this->rendermanager->record(this->swapchain.get());

    this->redraw();
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<::VulkanSurface> surface;
  VkSurfaceCapabilitiesKHR surface_capabilities{};
  std::shared_ptr<RenderpassObject> renderpass;
  std::unique_ptr<RenderManager> rendermanager;
  std::shared_ptr<SwapchainObject> swapchain;
};
