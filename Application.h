#pragma once

#include <Innovator/core/VulkanObjects.h>
#include <Viewer.h>
#include <vector>

class Window {
public:
  Window(HINSTANCE hinstance, WNDPROC WndProc, std::string name, uint32_t width, uint32_t height)
  {
    WNDCLASSEX wndclass {
      sizeof(WNDCLASSEX),                   // cbSize 
      CS_HREDRAW | CS_VREDRAW,              // style
      WndProc,                              // lpfnWndProc 
      0,                                    // cbClsExtra
      0,                                    // cbWndExtra
      hinstance,                            // hInstance 
      LoadIcon(nullptr, IDI_APPLICATION),   // hIcon 
      LoadCursor(nullptr, IDC_ARROW),       // hCursor 
      (HBRUSH)GetStockObject(WHITE_BRUSH),  // hbrBackground
      name.c_str(),                         // lpszMenuName
      name.c_str(),                         // lpszClassName 
      LoadIcon(nullptr, IDI_WINLOGO),       // hIconSm 
    };

    if (!RegisterClassEx(&wndclass)) {
      throw std::runtime_error("RegisterClassEx failed");
    }

    this->hwnd = CreateWindowEx(
      0,
      name.c_str(),
      name.c_str(),
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      100, 100,
      width, height,
      nullptr, nullptr,
      hinstance, nullptr);

    if (this->hwnd == nullptr) {
      throw std::runtime_error("CreateWindowEx failed.");
    }
  }

  ~Window()
  {
    DestroyWindow(this->hwnd);
  }

  void redraw()
  {
    RedrawWindow(this->hwnd, nullptr, nullptr, RDW_INTERNALPAINT);
  }

  HWND hwnd;
};

class VulkanSurfaceWin32 {
public:
  VulkanSurfaceWin32(const std::shared_ptr<VulkanInstance> & vulkan,
                     const std::shared_ptr<Window> & window,
                     HINSTANCE hinstance)
    : vulkan(vulkan)
  {
    VkWin32SurfaceCreateInfoKHR create_info {
      VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // sType 
      nullptr,                                         // pNext
      0,                                               // flags (reserved for future use)
      hinstance,                                       // hinstance 
      window->hwnd,                                    // hwnd
    };

    THROW_ON_ERROR(vkCreateWin32SurfaceKHR(this->vulkan->instance, &create_info, nullptr, &this->surface));
  }

  ~VulkanSurfaceWin32()
  {
    vkDestroySurfaceKHR(this->vulkan->instance, this->surface, nullptr);
  }

  VkSurfaceKHR surface;
  std::shared_ptr<VulkanInstance> vulkan;
};

class VulkanApplication {
public:
  VulkanApplication(HINSTANCE hinstance)
  {
    std::vector<const char *> instance_layers {
  #ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
  #endif
    };
    std::vector<const char *> instance_extensions {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME
    };

    VkApplicationInfo application_info{
      VK_STRUCTURE_TYPE_APPLICATION_INFO, // sType
      nullptr,                            // pNext
      "Innovator Viewer",                 // pApplicationName
      1,                                  // applicationVersion
      "Innovator",                        // pEngineName
      1,                                  // engineVersion
      VK_API_VERSION_1_0,                 // apiVersion
    };

    this->vulkan = std::make_shared<VulkanInstance>(
      application_info,
      instance_layers,
      instance_extensions);

#ifdef _DEBUG
    this->debugcb = std::make_unique<VulkanDebugCallback>(
      this->vulkan,
      VK_DEBUG_REPORT_WARNING_BIT_EXT | 
      VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | 
      VK_DEBUG_REPORT_ERROR_BIT_EXT,
      DebugCallback);
#endif

    this->window = std::make_shared<Window>(
      hinstance,
      WndProc,
      "Innovator Viewer",
      1000,
      700);

    SetWindowLongPtr(this->window->hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    this->surface = std::make_shared<VulkanSurfaceWin32>(
      this->vulkan,
      this->window,
      hinstance);

    this->viewer = std::make_unique<VulkanViewer>(
      this->vulkan,
      this->surface->surface);
  }

  static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if (uMsg == WM_CLOSE) {
      PostQuitMessage(0);
    }
    else {
      VulkanApplication * self = reinterpret_cast<VulkanApplication*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
      if (self) {
        switch (uMsg) {
        case WM_SIZE: 
          self->viewer->resize(); 
          break;
        case WM_PAINT: 
          self->viewer->render(); 
          break;
        case WM_KEYDOWN:
          self->viewer->keyDown(LOWORD(wParam));
          self->window->redraw();
          break;
        case WM_MOUSEMOVE:
          self->viewer->mouseMoved(LOWORD(lParam), HIWORD(lParam));
          self->window->redraw();
          break;
        case WM_LBUTTONUP:
          self->viewer->mouseReleased(LOWORD(lParam), HIWORD(lParam), 0); 
          break;
        case WM_RBUTTONUP:
          self->viewer->mouseReleased(LOWORD(lParam), HIWORD(lParam), 1);
          break;
        case WM_MBUTTONUP: 
          self->viewer->mouseReleased(LOWORD(lParam), HIWORD(lParam), 2); 
          break;
        case WM_LBUTTONDOWN: 
          self->viewer->mousePressed(LOWORD(lParam), HIWORD(lParam), 0); 
          break;
        case WM_RBUTTONDOWN: 
          self->viewer->mousePressed(LOWORD(lParam), HIWORD(lParam), 1); 
          break;
        case WM_MBUTTONDOWN: 
          self->viewer->mousePressed(LOWORD(lParam), HIWORD(lParam), 2); 
          break;
        default: 
          break;
        }
      }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }

  static VkBool32 DebugCallback(VkFlags flags,
                                VkDebugReportObjectTypeEXT type,
                                uint64_t src,
                                size_t location,
                                int32_t code,
                                const char *layer,
                                const char *msg,
                                void *)
  {
    std::string message;
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) { 
      message += "ERROR: "; 
    }
    if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) { 
      message += "DEBUG: "; 
    }
    if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) { 
      message += "WARNING: "; 
    }
    if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) { 
      message += "INFORMATION: "; 
    }
    if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) { 
      message += "PERFORMANCE_WARNING: "; 
    }
    message += std::string(layer) + " " + std::string(msg);
    std::cout << message << std::endl;
    return false;
  }

  void run()
  {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<Window> window;
  std::shared_ptr<VulkanSurfaceWin32> surface;
  std::unique_ptr<VulkanViewer> viewer;
  std::unique_ptr<VulkanDebugCallback> debugcb;
};
