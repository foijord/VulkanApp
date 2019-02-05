
#include <Viewer.h>
#include <Innovator/Nodes.h>
#include <Innovator/Core/File.h>
#include <Innovator/Core/Misc/Factory.h>

#include <QApplication>
#include <QWindow>
#include <QMouseEvent>

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <QX11Info>
#endif

#include <iostream>

static VkBool32 DebugCallback(
  VkFlags flags,
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

  VkExtent3D extent(size_t mip_level) const override
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

  size_t size(size_t level) const override
  {
    return this->image.sizeInBytes();
  }

  const void * data() const override
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

  void reloadShaders() const
  {
    // try {
    //   std::vector<std::shared_ptr<Shader>> shaders;
    //   SearchAction(this->scene, shaders);

    //   MemoryAllocator allocator(this->viewer->device);

    //   for (auto & shader : shaders) {
    //     shader->readFile();
    //     shader->alloc(&allocator);
    //   }

    //   this->viewer->pipeline();
    //   this->viewer->record();
    //   this->viewer->redraw();
    // }
    // catch (std::exception & e) {
    //   std::cerr << e.what() << std::endl;
    // }
  }

  void keyPressEvent(QKeyEvent * e) override
  {
    switch (e->key()) {
    case Qt::Key_R:
      this->reloadShaders();
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

int main(int argc, char *argv[])
{
  try {
    VulkanImageFactory::Register<QTextureImage>();

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

    VkApplicationInfo application_info{
      VK_STRUCTURE_TYPE_APPLICATION_INFO, // sType
      nullptr,                            // pNext
      "Innovator Viewer",                 // pApplicationName
      1,                                  // applicationVersion
      "Innovator",                        // pEngineName
      1,                                  // engineVersion
      VK_API_VERSION_1_0,                 // apiVersion
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
    QApplication app(argc, argv);

    VulkanWindow window;
    window.resize(512, 512);
    window.show();

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

    auto viewer = std::make_shared<VulkanViewer>(vulkan, surface, camera);

    window.camera = camera;
    window.viewer = viewer;

    File file;
    auto scene = file.open("Scenes/crate.scene");
    viewer->setSceneGraph(scene);

    return QApplication::exec();
  }
  catch (std::exception & e) {
    std::cerr << e.what() << std::endl;
  }
  return 1;
}
