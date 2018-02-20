#pragma once

#include <Innovator/core/VulkanObjects.h>
#include <Innovator/Actions.h>
#include <Innovator/Nodes.h>

#include <glm/glm.hpp>

#include <map>
#include <string>
#include <memory>
#include <vector>
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
        case WM_PAINT: self->render(); break;
        case WM_SIZE: self->resize(); break;
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

    std::vector<VkDeviceQueueCreateInfo> queue_create_info;
    float queue_priorities[1] = { 1.0f };

    auto add_queue_info = [&](uint32_t queue_index) {
      VkDeviceQueueCreateInfo create_info;
      ::memset(&create_info, 0, sizeof(VkDeviceQueueCreateInfo));
      create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      create_info.queueCount = 1;
      create_info.pQueuePriorities = queue_priorities;
      create_info.queueFamilyIndex = queue_index;
      queue_create_info.push_back(create_info);
    };

    // one generic queue for everything
    add_queue_info(physical_device.getQueueIndex(VK_QUEUE_GRAPHICS_BIT, this->surface->getPresentationFilter(physical_device)));
    //add_queue_info(physical_device.getQueueIndex(VK_QUEUE_COMPUTE_BIT));
    add_queue_info(physical_device.getQueueIndex(VK_QUEUE_TRANSFER_BIT));

    std::vector<std::string> device_layers = {
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };
    std::vector<std::string> device_extensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    this->device = std::make_shared<VulkanDevice>(physical_device, required_device_features, device_layers, device_extensions, queue_create_info);

    this->surface_format = this->surface->getSurfaceFormats(this->device->physical_device)[0];

    this->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
    std::vector<VkPresentModeKHR> present_modes = this->surface->getPresentModes(this->device->physical_device);
    if (std::find(present_modes.begin(), present_modes.end(), present_mode) == present_modes.end()) {
      throw std::runtime_error("surface does not support VK_PRESENT_MODE_MAILBOX_KHR");
    }

    this->handleeventaction = std::make_unique<HandleEventAction>();
    this->resize();
  }

  virtual ~VulkanViewer() {}

  void setSceneGraph(std::shared_ptr<class Node> scene)
  {
    this->renderaction = std::make_unique<RenderAction>(this->device, this->framebuffer_object, this->surface_capabilities.currentExtent);

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
      this->swapchain->swapBuffers();
    }
    catch (VkErrorOutOfDateException &) {
      this->resize();
    }
  }

  virtual void resize()
  {
    this->surface_capabilities = this->surface->getSurfaceCapabilities(this->device->physical_device);
    this->framebuffer_object = std::make_shared<FramebufferObject>(this->device, this->surface_format.format, this->surface_capabilities.currentExtent);
    this->renderaction = std::make_unique<RenderAction>(this->device, this->framebuffer_object, this->surface_capabilities.currentExtent);

    this->swapchain = std::make_unique<SwapchainObject>(
      this->vulkan,
      this->device,
      this->framebuffer_object->color_attachment,
      this->surface->surface,
      this->surface_capabilities,
      this->present_mode,
      this->surface_format,
      this->swapchain ? this->swapchain->swapchain->swapchain : nullptr);
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

  std::shared_ptr<class VulkanInstance> vulkan;
  std::unique_ptr<class VulkanDebugCallback> debugcb;
  std::unique_ptr<class VulkanSurfaceWin32> surface;
  std::shared_ptr<class VulkanDevice> device;
  std::shared_ptr<class FramebufferObject > framebuffer_object;
  std::unique_ptr<class RenderAction> renderaction;
  std::unique_ptr<class HandleEventAction> handleeventaction;
  std::unique_ptr<class SwapchainObject> swapchain;
  std::shared_ptr<class Separator> root;
  std::shared_ptr<class Camera> camera;

  VkPresentModeKHR present_mode;
  VkSurfaceFormatKHR surface_format;
  VkSurfaceCapabilitiesKHR surface_capabilities;

  int button;
  bool mouse_pressed;
  glm::vec2 mouse_pos;
};
