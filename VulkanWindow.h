#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/RenderManager.h>
#include <Innovator/misc/Defines.h>

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <Innovator/Win32Surface.h>
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <Innovator/XcbSurface.h>
#endif

#include <QWindow>
#include <QResizeEvent>

class VulkanWindow : public QWindow {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanWindow)
  ~VulkanWindow() = default;

  VulkanWindow(std::shared_ptr<VulkanInstance> vulkan,
               std::shared_ptr<VulkanDevice> device,
               std::shared_ptr<FramebufferAttachment> color_attachment,
               std::shared_ptr<Group> scene,
               std::shared_ptr<Camera> camera, 
               QWindow * parent = nullptr) :
    QWindow(parent),
    camera(std::move(camera))
  {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    this->surface = std::make_shared<::VulkanSurface>(
      vulkan,
      reinterpret_cast<HWND>(this->winId()),
      GetModuleHandle(nullptr));

#elif defined(VK_USE_PLATFORM_XCB_KHR)
    this->surface = std::make_shared<::VulkanSurface>(
      vulkan,
      static_cast<xcb_window_t>(this->winId()),
      QX11Info::connection());
#endif

	VkExtent2D extent{
		static_cast<uint32_t>(this->width()),
		static_cast<uint32_t>(this->height())
	};

    this->rendermanager = std::make_shared<RenderManager>(vulkan,
                                                          device,
                                                          extent);


    auto swapchain = std::make_shared<SwapchainObject>(color_attachment,
                                                       this->surface->surface,
                                                       VK_PRESENT_MODE_FIFO_KHR);

    this->root = std::make_shared<Group>();
    this->root->children = {
      scene,
      swapchain
    };

    this->rendermanager->init(this->root.get());
  }

  void resizeEvent(QResizeEvent * e) override
  {
    QWindow::resizeEvent(e);
	if (this->rendermanager) {
		VkExtent2D extent{
		  static_cast<uint32_t>(e->size().width()),
		  static_cast<uint32_t>(e->size().height())
		};
		this->rendermanager->resize(this->root.get(), extent);
	}
  }

  void keyPressEvent(QKeyEvent * e) override
  {
    switch (e->key()) {
    case Qt::Key_R:
      break;
    default:
      break;
    }
  }

  void mousePressEvent(QMouseEvent * e) override
  {
    this->button = e->button();
    this->mouse_pos = { static_cast<float>(e->x()), static_cast<float>(e->y()) };
    this->mouse_pressed = true;
  }

  void mouseReleaseEvent(QMouseEvent *) override
  {
    this->mouse_pressed = false;
  }

  void mouseMoveEvent(QMouseEvent * e) override
  {
    if (this->mouse_pressed) {
      const vec2d pos = { static_cast<double>(e->x()), static_cast<double>(e->y()) };
      vec2d dx = (this->mouse_pos - pos) * .01;
      dx.v[1] = -dx.v[1];
      switch (this->button) {
      case Qt::MiddleButton: this->camera->pan(dx); break;
      case Qt::LeftButton: this->camera->orbit(dx); break;
      case Qt::RightButton: this->camera->zoom(dx.v[1]); break;
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

  Qt::MouseButton button{ Qt::MouseButton::NoButton };
  bool mouse_pressed{ false };
  vec2d mouse_pos{};
};
