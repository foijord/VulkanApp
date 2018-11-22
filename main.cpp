
#define GLM_ENABLE_EXPERIMENTAL

#include <Viewer.h>
#include <Innovator/Nodes.h>
#include <Innovator/Core/File.h>
#include <Innovator/Core/Misc/Factory.h>

#include <QApplication>

#include <iostream>

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

    File file;
    const auto scene = file.open("Scenes/crate.scene");

    VulkanViewer viewer(vulkan);
    //viewer.setSceneGraph(create_octree());
    viewer.setSceneGraph(scene);
    viewer.resize(512, 512);
    viewer.show();

    return QApplication::exec();
  }
  catch (std::exception & e) {
    std::cerr << e.what() << std::endl;
  }
  return 1;
}
