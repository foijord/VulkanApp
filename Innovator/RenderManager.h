#pragma once

#include <Innovator/Defines.h>
#include <Innovator/Node.h>
#include <Innovator/State.h>
#include <Innovator/VulkanObjects.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>
#include <functional>

class SceneRenderer {
public:
  NO_COPY_OR_ASSIGNMENT(SceneRenderer)
  SceneRenderer() = delete;
  ~SceneRenderer() = default;

  explicit SceneRenderer(std::shared_ptr<VulkanInstance> vulkan,
                         std::shared_ptr<VulkanDevice> device,
                         VkExtent2D extent) :
    vulkan(std::move(vulkan)),
    device(std::move(device)),
    extent(extent)
  {}

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  RenderState state;
  VulkanCommandBuffers* command{ nullptr };
  VkExtent2D extent;  
};

class MemoryAllocator {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryAllocator)
  ~MemoryAllocator() = default;

  MemoryAllocator() = delete;

  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;
};

class RenderManager {
public:
  typedef std::function<void(RenderManager *)> alloc_callback;

  NO_COPY_OR_ASSIGNMENT(RenderManager)
  RenderManager() = delete;

  explicit RenderManager(std::shared_ptr<VulkanInstance> vulkan,
                         std::shared_ptr<VulkanDevice> device,
                         VkExtent2D extent) :
      vulkan(std::move(vulkan)),
      device(std::move(device)),
      extent(extent),
      fence(std::make_unique<VulkanFence>(this->device)),
      command(std::make_unique<VulkanCommandBuffers>(this->device)),
      pipelinecache(std::make_shared<VulkanPipelineCache>(this->device))
  {
    this->queue = this->device->getQueue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
  }

  virtual ~RenderManager() 
  {
    try {
      THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    } 
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void init(Node * root)
  {
    this->alloc(root);
    this->stage(root);
    this->pipeline(root);
    this->record(root);
  }

  void redraw(Node * root)
  {
    try {
      this->render(root);
      this->present(root);
    }
    catch (VkException &) {
      // recreate swapchain, try again next frame
      //this->resize(root, this->extent);
    }
  }

  void resize(Node * root, VkExtent2D extent)
  {
    // make sure all work submitted is done before we start recreating stuff
    THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));

    this->extent = extent;

    this->resize(root);
    this->record(root);
    this->redraw(root);
  }

  void begin_alloc()
  {
    this->imageobjects.clear();
    this->bufferobjects.clear();
  }

  void end_alloc()
  {
    for (auto & image_object : this->imageobjects) {
      const auto memory = std::make_shared<VulkanMemory>(
        this->device,
        image_object->memory_requirements.size,
        image_object->memory_type_index);

      const VkDeviceSize offset = 0;
      image_object->bind(memory, offset);
    }

    for (auto & buffer_object : this->bufferobjects) {
      const auto memory = std::make_shared<VulkanMemory>(
        this->device,
        buffer_object->memory_requirements.size,
        buffer_object->memory_type_index);

      const VkDeviceSize offset = 0;
      buffer_object->bind(memory, offset);
    }
  }

  void traverse(std::function<void()> action) 
  {
    this->state = State();

    this->begin_alloc();
    {
      VulkanCommandBufferScope scope(this->command->buffer());
      action();
    }
    this->end_alloc();

    FenceScope fence_scope(this->device->device, this->fence->fence);

    this->command->submit(this->queue,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                          this->fence->fence);
  }

  void alloc(Node * root)
  {    
    this->traverse([&]() {
      root->alloc(this);
    });
  }

  void resize(Node * root)
  {
    this->traverse([&]() {
      root->resize(this);
    });
  }

  void stage(Node * root)
  {
    this->traverse([&]() {
      root->stage(this);
    });
  }

  void pipeline(Node * root)
  {
    this->traverse([&]() {
      root->pipeline(this);
    });
  }

  void record(Node * root)
  {
    this->traverse([&]() {
      root->record(this);
    });
  }

  void render(Node * root) const
  {
    SceneRenderer renderer(this->vulkan, 
                           this->device, 
                           this->extent);

    root->render(&renderer);
  }

  void present(Node * root)
  {
    root->present(this);
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  VkExtent2D extent;
  VkQueue queue{ nullptr };

  State state;

  std::shared_ptr<VulkanFence> fence;
  std::unique_ptr<VulkanCommandBuffers> command;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;

  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;
};
