
#define GLM_ENABLE_EXPERIMENTAL

#include <Viewer.h>
#include <Innovator/Nodes.h>
#include <Innovator/Core/File.h>
#include <Innovator/Core/Misc/Factory.h>

#include <QApplication>
#include <QWindow>
#include <QMouseEvent>

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

class GLITextureImage : public VulkanTextureImage {
public:
  NO_COPY_OR_ASSIGNMENT(GLITextureImage)

  explicit GLITextureImage(const std::string & filename) :
    texture(gli::load(filename))
  {}
  virtual ~GLITextureImage() = default;

  VkExtent3D extent(size_t mip_level) const override
  {
    return {
      static_cast<uint32_t>(this->texture.extent(mip_level).x),
      static_cast<uint32_t>(this->texture.extent(mip_level).y),
      static_cast<uint32_t>(this->texture.extent(mip_level).z)
    };
  }

  uint32_t base_level() const override
  {
    return static_cast<uint32_t>(this->texture.base_level());
  }

  uint32_t levels() const override
  {
    return static_cast<uint32_t>(this->texture.levels());
  }

  uint32_t base_layer() const override
  {
    return static_cast<uint32_t>(this->texture.base_layer());
  }

  uint32_t layers() const override
  {
    return static_cast<uint32_t>(this->texture.layers());
  }

  size_t size() const override
  {
    return this->texture.size();
  }

  size_t size(size_t level) const override
  {
    return this->texture.size(level);
  }

  const void * data() const override
  {
    return this->texture.data();
  }

  VkFormat format() const override
  {
    return vulkan_format[this->texture.format()];
  }

  VkImageType image_type() const override
  {
    return vulkan_image_type[this->texture.target()];
  }

  VkImageViewType image_view_type() const override
  {
    return vulkan_image_view_type[this->texture.target()];
  }

  inline static std::map<gli::format, VkFormat> vulkan_format{
    { gli::format::FORMAT_R8_UNORM_PACK8, VkFormat::VK_FORMAT_R8_UNORM },
    { gli::format::FORMAT_RGBA_DXT5_UNORM_BLOCK16, VkFormat::VK_FORMAT_BC3_UNORM_BLOCK },
  };

  inline static std::map<gli::target, VkImageViewType> vulkan_image_view_type{
    { gli::target::TARGET_2D, VkImageViewType::VK_IMAGE_VIEW_TYPE_2D },
    { gli::target::TARGET_3D, VkImageViewType::VK_IMAGE_VIEW_TYPE_3D },
  };

  inline static std::map<gli::target, VkImageType> vulkan_image_type{
    { gli::target::TARGET_2D, VkImageType::VK_IMAGE_TYPE_2D },
    { gli::target::TARGET_3D, VkImageType::VK_IMAGE_TYPE_3D },
  };

  gli::texture texture;
};

class QTextureImage : public VulkanTextureImage {
public:
  NO_COPY_OR_ASSIGNMENT(QTextureImage)

  explicit QTextureImage(const std::string & filename) :
    image(QImage(filename.c_str()))
  {}
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
    return 0;
  }

  size_t size() const override
  {
    return 0;
  }

  size_t size(size_t level) const override
  {
    return 0;
  }

  const void * data() const override
  {
    return nullptr;
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

class QVulkanViewer : public QWindow {
public:
  NO_COPY_OR_ASSIGNMENT(QVulkanViewer)
  QVulkanViewer() = delete;
  ~QVulkanViewer() = default;

  QVulkanViewer(std::shared_ptr<VulkanInstance> vulkan, QWindow * parent = nullptr) :
    QWindow(parent)
  {
    this->camera = std::make_shared<Camera>(1000.0f, 0.1f, 4.0f / 3, 0.7f);
    this->camera->lookAt({ 0, 2, 4 }, { 0, 0, 0 }, { 0, 1, 0 });

    this->surface = std::make_shared<::VulkanSurface>(
      vulkan,
      reinterpret_cast<HWND>(this->winId()),
      GetModuleHandle(nullptr));

    this->viewer = std::make_shared<VulkanViewer>(vulkan, this->surface, this->camera);
  }

  void loadSceneGraph(const std::string & filename)
  {
    File file;
    this->scene = file.open("Scenes/crate.scene");
    this->viewer->setSceneGraph(scene);
  }

  void resizeEvent(QResizeEvent * e) override
  {
    QWindow::resizeEvent(e);
    this->viewer->resize();
  }

  void reloadShaders() const
  {
    try {
      std::vector<std::shared_ptr<Shader>> shaders;
      SearchAction(this->scene, shaders);

      MemoryAllocator allocator(this->viewer->device);

      for (auto & shader : shaders) {
        shader->readFile();
        shader->alloc(&allocator);
      }

      this->viewer->pipeline();
      this->viewer->record();
      this->viewer->redraw();
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
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
      dx[1] = -dx[1];
      switch (this->button) {
      case Qt::MiddleButton: this->camera->pan(dx); break;
      case Qt::LeftButton: this->camera->orbit(dx); break;
      case Qt::RightButton: this->camera->zoom(dx[1]); break;
      default: break;
      }
      this->mouse_pos = pos;
      this->viewer->redraw();
    }
  }

  std::shared_ptr<::VulkanSurface> surface;
  std::shared_ptr<Camera> camera;
  std::shared_ptr<VulkanViewer> viewer;
  std::shared_ptr<Separator> scene;

  Qt::MouseButton button{ Qt::MouseButton::NoButton };
  bool mouse_pressed{ false };
  vec2d mouse_pos{};
};

class VulkanApplication : public QApplication {
public:
  VulkanApplication(int argc, char* argv[]) 
    : QApplication(argc, argv) 
  {}
  
  bool notify(QObject * receiver, QEvent * event) override {
    try {
      return QApplication::notify(receiver, event);
    } 
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
    return false;
  }
};

int main(int argc, char *argv[])
{
  try {
    VulkanImageFactory::Register<GLITextureImage>();

    std::vector<const char *> instance_layers{
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };
    std::vector<const char *> instance_extensions{
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

    QVulkanViewer viewer(vulkan);
    viewer.loadSceneGraph("Scenes/crate.scene");
    viewer.resize(512, 512);
    viewer.show();

    return QApplication::exec();
  }
  catch (std::exception & e) {
    std::cerr << e.what() << std::endl;
  }
  return 1;
}
