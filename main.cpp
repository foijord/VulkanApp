
#define GLM_ENABLE_EXPERIMENTAL

#include <Viewer.h>
#include <Innovator/Nodes.h>
#include <Innovator/Actions.h>

#include <glm/glm.hpp>

#include <iostream>

using namespace std;
using namespace glm;

class Crate : public Separator {
public:
  Crate()
  {
    this->children = {
      std::make_shared<LayoutBinding>(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
      std::make_shared<Sampler>(),
      std::make_shared<Texture>("Textures/crate.dds"),
      std::make_shared<Shader>("Shaders/vertex.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
      std::make_shared<Shader>("Shaders/fragment.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
      std::make_shared<Box>(0, 0),
    };
  }
  virtual ~Crate() {}
};

class FullScreenTriangle : public Separator {
public:
  FullScreenTriangle() 
  {
    auto rasterization_state = std::make_shared<RasterizationState>();
    rasterization_state->state.cullMode = VK_CULL_MODE_FRONT_BIT;

    this->children = {
      rasterization_state,
      std::make_shared<Shader>("Shaders/fullscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
      std::make_shared<Shader>("Shaders/fullscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
      std::make_shared<DrawCommand>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3)
    };
  }
  virtual ~FullScreenTriangle() {}
};

class ComputeTest : public Node {
public:
  ComputeTest()
    : t(0)
  {
    auto data0 = std::make_shared<Buffer<vec4>>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    auto data1 = std::make_shared<Buffer<vec4>>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    data0->values.resize(128 * 128);
    data1->values.resize(128 * 128);

    std::fill(data0->values.begin(), data0->values.end(), vec4(0));
    std::fill(data1->values.begin(), data1->values.end(), vec4(0));

    auto layout0 = std::make_shared<LayoutBinding>(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL);
    auto layout1 = std::make_shared<LayoutBinding>(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_ALL);

    auto create_step = [&](auto layout0, auto data0, auto layout1, auto data1) {
      return 
        Separator::create({
          layout0, data0,
          layout1, data1,
          Separator::create({
            std::make_shared<Shader>("Shaders/compute.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT),
            std::make_shared<ComputeCommand>(4, 4, 1)
          }),
          Separator::create({
            std::make_shared<CullMode>(VK_CULL_MODE_FRONT_BIT),
            std::make_shared<Shader>("Shaders/fullscreen.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
            std::make_shared<Shader>("Shaders/fullscreen.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
            std::make_shared<DrawCommand>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3)
          }),
        });
      };

    this->steps[0] = create_step(layout0, data0, layout1, data1);
    this->steps[1] = create_step(layout0, data1, layout1, data0);
  }
  virtual ~ComputeTest() {}

  virtual void traverse(RenderAction * action)
  {
    this->steps[t++ % 2]->traverse(action);
  }

  shared_ptr<Separator> steps[2];
  size_t t;
};


class VolumeViewer : public VulkanViewer {
public:
  VolumeViewer(HINSTANCE hinstance)
    : VulkanViewer(hinstance, 1000, 800)
  {
    this->setSceneGraph(std::make_shared<Crate>());
  }

  virtual void keyDown(uint32_t key)
  {
    switch (key) {
    case 'S': {
      this->setSceneGraph(std::make_shared<ComputeTest>());
      this->redraw();
      break;
    }
    default:
      VulkanViewer::keyDown(key);
      break;
    }
  }
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
  try {
    VolumeViewer viewer(hInstance);
    viewer.run();
  }
  catch (std::exception & e) {
    MessageBox(nullptr, e.what(), "Alert", MB_OK);
  }
  return 1;
}
