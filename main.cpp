
#define GLM_ENABLE_EXPERIMENTAL

#include <Application.h>
#include <Innovator/Nodes.h>
#include <Innovator/Core/File.h>
#include <iostream>

int main()
{
  try {
    File file;
    VulkanApplication app(GetModuleHandle(nullptr));

    const auto scene = file.open("Scenes/crate.scene");
    app.viewer->setSceneGraph(scene);
    app.run();
  }
  catch (std::exception & e) {
    std::cout << e.what() << std::endl;
  }
  return 1;
}
