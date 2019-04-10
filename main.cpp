
#include <Viewer.h>
#include <Innovator/File.h>
#include <Innovator/Misc/Factory.h>
#include <Innovator/Misc/Defines.h>

#include <QApplication>
#include <QWindow>
#include <QMouseEvent>

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <QX11Info>
#endif

#include <iostream>

static VkBool32 DebugCallback(
  VkFlags flags,
  VkDebugReportObjectTypeEXT,
  uint64_t,
  size_t,
  int32_t,
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
  return VK_FALSE;
}

class QTextureImage : public VulkanTextureImage {
public:
  NO_COPY_OR_ASSIGNMENT(QTextureImage)

  explicit QTextureImage(const std::string & filename) :
    image(QImage(filename.c_str()))
  {
    this->image = this->image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
  }

  virtual ~QTextureImage() = default;

  VkExtent3D extent(size_t) const override
  {
    return {
      static_cast<uint32_t>(this->image.size().width()),
      static_cast<uint32_t>(this->image.size().height()),
      1 
    };
  }

  uint32_t base_level() const override
  {
    return 0;
  }

  uint32_t levels() const override
  {
    return 1;
  }

  uint32_t base_layer() const override
  {
    return 0;
  }

  uint32_t layers() const override
  {
    return 1;
  }

  size_t size() const override
  {
    return this->image.sizeInBytes();
  }

  size_t size(size_t) const override
  {
    return this->image.sizeInBytes();
  }

  const unsigned char * data() const override
  {
    return this->image.bits();
  }

  VkFormat format() const override
  {
    return VK_FORMAT_R8G8B8A8_UNORM;
  }

  VkImageType image_type() const override
  {
    return VK_IMAGE_TYPE_2D;
  }

  VkImageViewType image_view_type() const override
  {
    return VK_IMAGE_VIEW_TYPE_2D;
  }

  QImage image;
};

class VulkanWindow : public QWindow {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanWindow)
  ~VulkanWindow() = default;

  VulkanWindow(QWindow * parent = nullptr) : QWindow(parent) {}

  void resizeEvent(QResizeEvent * e) override
  {
    QWindow::resizeEvent(e);
    if (this->viewer) {
      this->viewer->resize();
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
      this->viewer->redraw();
    }
  }

  std::shared_ptr<Camera> camera;
  std::shared_ptr<VulkanViewer> viewer;

  Qt::MouseButton button{ Qt::MouseButton::NoButton };
  bool mouse_pressed{ false };
  vec2d mouse_pos{};
};

class VulkanApplication : public QApplication {
public:
  VulkanApplication(int& argc, char** argv) :
    QApplication(argc, argv) 
  {}

  bool notify(QObject* receiver, QEvent* event)
  {
    bool done = true;
    try {
      done = QApplication::notify(receiver, event);
    } 
    catch (VkException & e) {
      std::cerr << std::string("caught exception in VulkanApplication::notify(): ") + typeid(e).name() << std::endl;
    }
    catch (std::exception & e) {
      std::cerr << std::string("caught exception in VulkanApplication::notify(): ") + typeid(e).name() << std::endl;
    }

    return done;
  }
};

int main(int argc, char *argv[])
{
  try {
    VulkanImageFactory::Register<QTextureImage>();

    VkApplicationInfo application_info{
      VK_STRUCTURE_TYPE_APPLICATION_INFO, // sType
      nullptr,                            // pNext
      "Innovator Viewer",                 // pApplicationName
      1,                                  // applicationVersion
      "Innovator",                        // pEngineName
      1,                                  // engineVersion
      VK_API_VERSION_1_0,                 // apiVersion
    };

    std::vector<const char *> instance_layers{
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    std::vector<const char *> instance_extensions{
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
      VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif
    };

    auto vulkan = std::make_shared<VulkanInstance>(
      application_info,
      instance_layers,
      instance_extensions);

#ifdef _DEBUG
    auto debugcb = std::make_unique<VulkanDebugCallback>(
      vulkan,
      VK_DEBUG_REPORT_WARNING_BIT_EXT |
      VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
      VK_DEBUG_REPORT_ERROR_BIT_EXT,
      DebugCallback);
#endif

    VulkanApplication app(argc, argv);
    VulkanWindow window;

#if defined(VK_USE_PLATFORM_WIN32_KHR)
    auto surface = std::make_shared<::VulkanSurface>(
      vulkan,
      reinterpret_cast<HWND>(window.winId()),
      GetModuleHandle(nullptr));

#elif defined(VK_USE_PLATFORM_XCB_KHR)
    auto surface = std::make_shared<::VulkanSurface>(
      vulkan,
      static_cast<xcb_window_t>(window.winId()),
      QX11Info::connection());
#endif

    auto camera = std::make_shared<Camera>(1000.0f, 0.1f, 4.0f / 3, 0.7f);
    camera->lookAt({ 0, 2, 4 }, { 0, 0, 0 }, { 0, 1, 0 });

    VkPhysicalDeviceFeatures required_device_features;
    ::memset(&required_device_features, VK_FALSE, sizeof(VkPhysicalDeviceFeatures));
    required_device_features.geometryShader = VK_TRUE;
    required_device_features.tessellationShader = VK_TRUE;
    required_device_features.textureCompressionBC = VK_TRUE;
    required_device_features.fragmentStoresAndAtomics = VK_TRUE;

    auto physical_device = vulkan->selectPhysicalDevice(required_device_features);

    VkQueueFlags queue_flags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{ 
      physical_device.getQueueCreateInfo(queue_flags, surface->surface)
    };

    std::vector<const char *> device_layers{
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    std::vector<const char *> device_extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    auto device = std::make_shared<VulkanDevice>(physical_device,
                                                 required_device_features,
                                                 device_layers,
                                                 device_extensions,
                                                 queue_create_infos);

    auto viewer = std::make_shared<VulkanViewer>(vulkan, 
                                                 device, 
                                                 surface, 
                                                 camera);

    window.camera = camera;
    window.viewer = viewer;

    auto scene = eval_file("crate.scene");
    viewer->setSceneGraph(scene);

    window.resize(512, 512);
    window.show();

    return VulkanApplication::exec();
  }
  catch (std::exception & e) {
    std::cerr << std::string("caught exception in main(): ") + typeid(e).name() << std::endl;
  }
  return 1;
}
