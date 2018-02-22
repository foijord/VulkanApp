#pragma once

#include <Innovator/core/VulkanObjects.h>
#include <Innovator/Actions.h>
#include <Innovator/Nodes.h>

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>

class VulkanSurfaceWin32 {
public:
  VulkanSurfaceWin32(const std::shared_ptr<VulkanInstance> & vulkan, HWND hwnd, HINSTANCE hinstance)
    : vulkan(vulkan)
  {
    VkWin32SurfaceCreateInfoKHR create_info;
    ::memset(&create_info, 0, sizeof(VkWin32SurfaceCreateInfoKHR));
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.hwnd = hwnd;
    create_info.hinstance = hinstance;

    THROW_ERROR(vkCreateWin32SurfaceKHR(this->vulkan->instance, &create_info, nullptr, &this->surface));
  }

  ~VulkanSurfaceWin32()
  {
    vkDestroySurfaceKHR(this->vulkan->instance, this->surface, nullptr);
  }

  std::vector<VkPresentModeKHR> getPresentModes(const VulkanPhysicalDevice & device)
  {
    uint32_t mode_count;
    THROW_ERROR(this->vulkan->vkGetPhysicalDeviceSurfacePresentModes(device.device, this->surface, &mode_count, nullptr));
    std::vector<VkPresentModeKHR> modes(mode_count);
    THROW_ERROR(this->vulkan->vkGetPhysicalDeviceSurfacePresentModes(device.device, this->surface, &mode_count, modes.data()));
    return modes;
  }

  VkSurfaceCapabilitiesKHR getSurfaceCapabilities(const VulkanPhysicalDevice & device)
  {
    VkSurfaceCapabilitiesKHR capabilities;
    THROW_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceCapabilities(device.device, this->surface, &capabilities));
    return capabilities;
  }

  std::vector<VkSurfaceFormatKHR> getSurfaceFormats(const VulkanPhysicalDevice & device)
  {
    uint32_t format_count;
    THROW_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceFormats(device.device, this->surface, &format_count, nullptr));
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    THROW_ERROR(this->vulkan->vkGetPhysicalDeviceSurfaceFormats(device.device, this->surface, &format_count, formats.data()));
    return formats;
  }

  std::vector<VkBool32> getPresentationFilter(const VulkanPhysicalDevice & device)
  {
    std::vector<VkBool32> filter(device.queue_family_properties.size());
    for (uint32_t i = 0; i < device.queue_family_properties.size(); i++) {
      vkGetPhysicalDeviceSurfaceSupportKHR(device.device, i, this->surface, &filter[i]);
    }
    return filter;
  }

  VkSurfaceKHR surface;
  std::shared_ptr<VulkanInstance> vulkan;
};

class Window {
public:
  Window(HINSTANCE hinstance, std::string name, uint32_t width, uint32_t height)
  {
    WNDCLASSEX wndclass;
    memset(&wndclass, 0, sizeof(WNDCLASSEX));
    wndclass.cbSize = sizeof(WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.hInstance = hinstance;
    wndclass.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszClassName = name.c_str();
    wndclass.hIconSm = LoadIcon(nullptr, IDI_WINLOGO);

    if (!RegisterClassEx(&wndclass)) {
      throw std::runtime_error("RegisterClassEx failed");
    }
    this->window = CreateWindowEx(0, name.c_str(), name.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, width, height, nullptr, nullptr, hinstance, nullptr);
    if (this->window == nullptr) {
      throw std::runtime_error("CreateWindowEx failed.");
    }
    SetWindowLongPtr(this->window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
  }

  virtual ~Window()
  {
    DestroyWindow(this->window);
  }


  virtual void render() = 0;
  virtual void resize() = 0;
  virtual void keyDown(uint32_t key) = 0;
  virtual void mousePressed(uint32_t x, uint32_t y, int button) = 0;
  virtual void mouseReleased(uint32_t x, uint32_t y, int button) = 0;
  virtual void mouseMoved(uint32_t x, uint32_t y) = 0;

  static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if (uMsg == WM_CLOSE) {
      PostQuitMessage(0);
    }
    else {
      Window * self = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
      if (self != nullptr) {
        switch (uMsg) {
        case WM_SIZE: self->resize(); break;
        case WM_PAINT: self->render(); break;
        case WM_KEYDOWN: self->keyDown(LOWORD(wParam)); break;
        case WM_MOUSEMOVE: self->mouseMoved(LOWORD(lParam), HIWORD(lParam)); break;
        case WM_LBUTTONUP: self->mouseReleased(LOWORD(lParam), HIWORD(lParam), 0); break;
        case WM_RBUTTONUP: self->mouseReleased(LOWORD(lParam), HIWORD(lParam), 1); break;
        case WM_MBUTTONUP: self->mouseReleased(LOWORD(lParam), HIWORD(lParam), 2); break;
        case WM_LBUTTONDOWN: self->mousePressed(LOWORD(lParam), HIWORD(lParam), 0); break;
        case WM_RBUTTONDOWN: self->mousePressed(LOWORD(lParam), HIWORD(lParam), 1); break;
        case WM_MBUTTONDOWN: self->mousePressed(LOWORD(lParam), HIWORD(lParam), 2); break;
        default: break;
        }
      }
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
  }

  void run()
  {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  void redraw()
  {
    RedrawWindow(this->window, nullptr, nullptr, RDW_INTERNALPAINT);
  }

  static VkBool32 DebugCallback(VkFlags flags, VkDebugReportObjectTypeEXT type, uint64_t src, size_t location, int32_t code, const char *layer, const char *msg, void *)
  {
    std::string message;
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) { message += "ERROR: "; }
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) { message += "DEBUG: "; }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) { message += "WARNING: "; }
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) { message += "INFORMATION: "; }
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) { message += "PERFORMANCE_WARNING: "; }

    message += std::string(layer) + " " + std::string(msg);
    MessageBox(nullptr, message.c_str(), "Alert", MB_OK);
    return false;
  }

protected:
  HWND window;
};

class VulkanViewer : public Window {
public:
  VulkanViewer(HINSTANCE hinstance, uint32_t width, uint32_t height)
    : Window(hinstance, "Innovator Viewer", width, height)
  {
    std::vector<std::string> instance_layers = {
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };
    std::vector<std::string> instance_extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    VkApplicationInfo application_info;
    ::memset(&application_info, 0, sizeof(VkApplicationInfo));

    application_info.pApplicationName = "Innovator Viewer";
    application_info.applicationVersion = 1;
    application_info.pEngineName = "Innovator";
    application_info.engineVersion = 1;
    application_info.apiVersion = VK_API_VERSION_1_0;

    this->vulkan = std::make_unique<VulkanInstance>(application_info, instance_layers, instance_extensions);
    this->debugcb = std::make_unique<VulkanDebugCallback>(this->vulkan, VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT, DebugCallback);
    this->surface = std::make_unique<VulkanSurfaceWin32>(this->vulkan, this->window, hinstance);

    VkPhysicalDeviceFeatures required_device_features;
    ::memset(&required_device_features, 0, sizeof(VkPhysicalDeviceFeatures));
    required_device_features.geometryShader = VK_TRUE;
    required_device_features.tessellationShader = VK_TRUE;
    required_device_features.textureCompressionBC = VK_TRUE;

    VulkanPhysicalDevice physical_device = this->vulkan->selectPhysicalDevice(required_device_features);

    float queue_priorities[1] = { 1.0f };
    uint32_t queue_index = physical_device.getQueueIndex(
      VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT,
      this->surface->getPresentationFilter(physical_device));

    std::vector<VkDeviceQueueCreateInfo> queue_create_info;
    queue_create_info.push_back(VkDeviceQueueCreateInfo{
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,   // sType
      nullptr,                                      // pNext
      0,                                            // flags
      queue_index,                                  // queueFamilyIndex
      1,                                            // queueCount   
      queue_priorities                              // pQueuePriorities
      });

    std::vector<std::string> device_layers = {
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };
    std::vector<std::string> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    this->device = std::make_shared<VulkanDevice>(physical_device, required_device_features, device_layers, device_extensions, queue_create_info);
    this->semaphore = std::make_unique<VulkanSemaphore>(this->device);
    this->command = std::make_unique<VulkanCommandBuffers>(this->device);
    this->surface_format = this->surface->getSurfaceFormats(this->device->physical_device)[0];

    this->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    std::vector<VkPresentModeKHR> present_modes = this->surface->getPresentModes(this->device->physical_device);
    if (std::find(present_modes.begin(), present_modes.end(), present_mode) == present_modes.end()) {
      throw std::runtime_error("surface does not support VK_PRESENT_MODE_MAILBOX_KHR");
    }
    this->handleeventaction = std::make_unique<HandleEventAction>();

    this->depth_format = VK_FORMAT_D32_SFLOAT;
    this->color_format = this->surface_format.format;

    std::vector<VkAttachmentDescription> attachment_descriptions = { {
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


    VkAttachmentReference depth_stencil_attachment = {
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    std::vector<VkAttachmentReference> color_attachments = {
      { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL }
    };

    std::vector<VkSubpassDescription> subpass_descriptions = { {
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

  virtual ~VulkanViewer()
  {
    try {
      // make sure all work submitted to GPU is done before we start deleting stuff...
      THROW_ERROR(vkDeviceWaitIdle(this->device->device));
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void setSceneGraph(std::shared_ptr<class Node> scene)
  {
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

  virtual void render()
  {
    this->renderaction->apply(this->root);
    try {
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
      THROW_ERROR(this->vulkan->vkQueuePresent(this->device->default_queue, &present_info));
    }
    catch (VkErrorOutOfDateException &) {
      this->resize();
    }
  }

  virtual void resize()
  {
    // make sure all work submitted is done before we start recreating stuff
    THROW_ERROR(vkDeviceWaitIdle(this->device->device));

    this->surface_capabilities = this->surface->getSurfaceCapabilities(this->device->physical_device);
    
    VkExtent2D extent2d = this->surface_capabilities.currentExtent;
    VkExtent3D extent3d = { extent2d.width, extent2d.height, 1 };

    std::vector<VkImageMemoryBarrier> memory_barriers;
    {
      VkImageSubresourceRange subresource_range = { 
        VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 
      };
      VkComponentMapping component_mapping = { 
        VK_COMPONENT_SWIZZLE_R, 
        VK_COMPONENT_SWIZZLE_G, 
        VK_COMPONENT_SWIZZLE_B, 
        VK_COMPONENT_SWIZZLE_A 
      };

      this->color_buffer = std::make_unique<ImageObject>(
        this->device,
        color_format,
        extent3d,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_TYPE_2D,
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
      VkImageSubresourceRange subresource_range = { 
        VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 
      };
      VkComponentMapping component_mapping = {
        VK_COMPONENT_SWIZZLE_IDENTITY, 
        VK_COMPONENT_SWIZZLE_IDENTITY, 
        VK_COMPONENT_SWIZZLE_IDENTITY, 
        VK_COMPONENT_SWIZZLE_IDENTITY 
      };

      this->depth_buffer = std::make_unique<ImageObject>(
        this->device,
        depth_format,
        extent3d,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_TYPE_2D,
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

    std::vector<VkImageView> framebuffer_attachments = {
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
      this->surface->surface,
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

    this->command->begin();
    vkCmdPipelineBarrier(
      this->command->buffer(),
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0, 0, nullptr, 0, nullptr,
      static_cast<uint32_t>(memory_barriers.size()),
      memory_barriers.data());

    this->command->end();
    this->command->submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);

    VkImageSubresourceRange subresource_range = {
      VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
    };

    std::vector<VkImageMemoryBarrier> src_image_barriers = { {
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
    }, {
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

      CommandBufferRecorder buffer_scope(this->swap_buffers_command.get(), i);

      vkCmdPipelineBarrier(buffer_scope.command(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(src_image_barriers.size()),
        src_image_barriers.data());

      VkImageSubresourceLayers subresource_layers = {
        subresource_range.aspectMask,     // aspectMask
        subresource_range.baseMipLevel,   // mipLevel
        subresource_range.baseArrayLayer, // baseArrayLayer
        subresource_range.layerCount      // layerCount;
      };
      VkOffset3D offset = {
        0, 0, 0
      };
      VkImageCopy image_copy = {
        subresource_layers,             // srcSubresource
        offset,                         // srcOffset
        subresource_layers,             // dstSubresource
        offset,                         // dstOffset
        extent3d                        // extent
      };

      vkCmdCopyImage(buffer_scope.command(), this->color_buffer->image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain_images[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);

      vkCmdPipelineBarrier(buffer_scope.command(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(dst_image_barriers.size()),
        dst_image_barriers.data());
    }
  }

  virtual void keyDown(uint32_t key)
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
    this->redraw();
  }

  virtual void mousePressed(uint32_t x, uint32_t y, int button)
  {
    this->button = button;
    this->mouse_pos = glm::vec2(x, y);
    this->mouse_pressed = true;
  }

  virtual void mouseReleased(uint32_t x, uint32_t y, int button)
  {
    this->mouse_pressed = false;
  }

  virtual void mouseMoved(uint32_t x, uint32_t y)
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
      this->redraw();
    }
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::unique_ptr<VulkanDebugCallback> debugcb;
  std::unique_ptr<VulkanSurfaceWin32> surface;
  std::shared_ptr<VulkanDevice> device;
  std::unique_ptr<VulkanCommandBuffers> command;
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

  VkPresentModeKHR present_mode;
  VkSurfaceFormatKHR surface_format;
  VkSurfaceCapabilitiesKHR surface_capabilities;

  int button;
  bool mouse_pressed;
  glm::vec2 mouse_pos;
};
