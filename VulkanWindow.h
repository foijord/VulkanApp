#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/RenderManager.h>
#include <Innovator/misc/Defines.h>
#include <Innovator/VulkanSurface.h>

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

namespace m = Innovator::Math;

class Window {
public:
  Window()
  {
    this->hInstance = GetModuleHandle(NULL);
    TCHAR szWindowClass[] = _T("DesktopApp");
    TCHAR szTitle[] = _T("Vulkan Window");

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) {
      throw std::runtime_error("unable to register window class");
    }

    this->hWnd = CreateWindow(
      szWindowClass,
      _T("Windows Desktop Guided Tour Application"),
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT,
      640, 480,
      NULL,
      NULL,
      hInstance,
      this);

    if (!hWnd) {
      throw std::runtime_error("Call to CreateWindow failed!");
    }
  }

  virtual ~Window() = default;

  virtual void redraw() = 0;
  virtual void resize(int width, int height) = 0;
  virtual void mousePressed(int x, int y, int button) = 0;
  virtual void mouseReleased() = 0;
  virtual void mouseMoved(int x, int y) = 0;

  LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    switch (message) {
    case WM_PAINT:
      this->redraw();
      break;
    case WM_SIZE:
      this->resize(LOWORD(lParam), HIWORD(lParam));
      break;
    case WM_LBUTTONDOWN:
      this->mousePressed(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 0);
      break;
    case WM_MBUTTONDOWN:
      this->mousePressed(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 1);
      break;
    case WM_RBUTTONDOWN:
      this->mousePressed(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), 2);
      break;
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
      this->mouseReleased();
      break;
    case WM_MOUSEMOVE:
      this->mouseMoved(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
      break;
    }
    return 0;
  }


  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    Window* self = nullptr;
    if (message == WM_NCCREATE) {
      LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
      self = static_cast<Window*>(lpcs->lpCreateParams);
      SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    else {
      self = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }
    if (self) {
      return self->wndProc(hWnd, message, wParam, lParam);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
  }


  int show()
  {
    ShowWindow(this->hWnd, SW_SHOWDEFAULT);
    UpdateWindow(this->hWnd);

    // Main message loop:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    return (int)msg.wParam;
  }

protected:
  HWND hWnd;
  HMODULE hInstance;
};


class VulkanWindow : public Window {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanWindow)
    ~VulkanWindow() = default;

  VulkanWindow(std::shared_ptr<VulkanInstance> vulkan,
               std::shared_ptr<VulkanDevice> device,
               std::shared_ptr<FramebufferAttachment> color_attachment,
               std::shared_ptr<Group> scene,
               std::shared_ptr<Camera> camera) :
    camera(std::move(camera))
  {
    this->surface = std::make_shared<::VulkanSurface>(vulkan, this->hWnd, this->hInstance);
    VkSurfaceCapabilitiesKHR surface_capabilities = this->surface->getSurfaceCapabilities(device);
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    this->surface->checkPresentModeSupport(device, present_mode);
    VkSurfaceFormatKHR surface_format = this->surface->getSupportedSurfaceFormat(device, color_attachment->format);

    this->rendermanager = std::make_shared<RenderManager>(
      vulkan,
      device,
      surface_capabilities.currentExtent);

    auto swapchain = std::make_shared<SwapchainObject>(
      color_attachment,
      this->surface->surface,
      surface_format,
      present_mode);

    auto offscreen = std::make_shared<OffscreenImage>(color_attachment);

    this->root = std::make_shared<Group>();
    this->root->children = {
      scene,
      swapchain,
      //offscreen
    };

    this->rendermanager->init(this->root.get());
  }

  void redraw() override
  {
    this->rendermanager->redraw(this->root.get());
  }

  void resize(int width, int height) override
  {
    VkExtent2D extent{
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height)
    };
    this->rendermanager->resize(this->root.get(), extent);
  }

  void mousePressed(int x, int y, int button)
  {
    this->button = button;
    this->mouse_pos = { static_cast<float>(x), static_cast<float>(y) };
    this->mouse_pressed = true;
  }

  void mouseReleased() override
  {
    this->mouse_pressed = false;
  }

  void mouseMoved(int x, int y)
  {
    if (this->mouse_pressed) {
      const m::vec2d pos = { static_cast<double>(x), static_cast<double>(y) };
      m::vec2d dx = (this->mouse_pos - pos) * .01;
      dx.v[1] = -dx.v[1];
      switch (this->button) {
      case 0: this->camera->orbit(dx); break;
      case 1: this->camera->pan(dx); break;
      case 2: this->camera->zoom(dx.v[1]); break;
      default: break;
      }
      this->mouse_pos = pos;
      this->rendermanager->redraw(this->root.get());
    }
  }

  std::shared_ptr<::VulkanSurface> surface;
  std::shared_ptr<Group> root;
  std::shared_ptr<RenderManager> rendermanager;
  std::shared_ptr<Camera> camera;

  int button;
  bool mouse_pressed{ false };
  m::vec2d mouse_pos{};
};
