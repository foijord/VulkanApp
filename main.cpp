
#define GLM_ENABLE_EXPERIMENTAL

#include <Viewer.h>
#include <Innovator/Nodes.h>
#include <Innovator/Core/File.h>

#include <QApplication>

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
    viewer.setSceneGraph(scene);
    viewer.resize(1000, 700);
    viewer.show();

    return QApplication::exec();
  }
  catch (std::exception & e) {
    std::cerr << e.what() << std::endl;
  }
  return 1;
}
