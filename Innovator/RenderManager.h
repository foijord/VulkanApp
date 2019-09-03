#pragma once

#include <Innovator/Misc/Defines.h>
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
                         VulkanCommandBuffers * command,
                         VkExtent2D extent) :
    vulkan(std::move(vulkan)),
    device(std::move(device)),
    command(command),
    extent(extent)
  {}

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  RenderState state;
  VulkanCommandBuffers * command;
  VkExtent2D extent;  
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
      render_fence(std::make_unique<VulkanFence>(this->device)),
      stage_fence(std::make_unique<VulkanFence>(this->device)),
      command(std::make_unique<VulkanCommandBuffers>(this->device)),
      render_command(std::make_unique<VulkanCommandBuffers>(this->device)),
      pipelinecache(std::make_shared<VulkanPipelineCache>(this->device)),
      rendering_finished(std::make_unique<VulkanSemaphore>(this->device))
  {
    this->default_queue = this->device->getQueue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
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
    this->alloc_callbacks.clear();
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
    for (auto & alloc_callback : this->alloc_callbacks) {
      alloc_callback(this);
    }
  }

  void alloc(Node * root)
  {    
    this->state = State();

    this->begin_alloc();
    {
      VulkanCommandBufferScope scope(this->command->buffer());
      root->alloc(this);
    }
    this->end_alloc();
  }

  void resize(Node * root)
  {
    this->state = State();

    this->begin_alloc();
    {
      VulkanCommandBufferScope scope(this->command->buffer());
      root->resize(this);
    }

    this->end_alloc();

    FenceScope fence_scope(this->device->device, this->stage_fence->fence);
    
    this->command->submit(this->default_queue,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                          this->stage_fence->fence);
  }

  void stage(Node * root)
  {
    this->state = State();
    {
      VulkanCommandBufferScope scope(this->command->buffer());
      root->stage(this);
    }
    FenceScope fence_scope(this->device->device, this->stage_fence->fence);
    
    this->command->submit(this->default_queue,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                          this->stage_fence->fence);
  }

  void pipeline(Node * root)
  {
    this->state = State();
    root->pipeline(this);
  }

  void record(Node * root)
  {
    this->state = State();
    root->record(this);
  }

  void render(Node * root) const
  {
    SceneRenderer renderer(this->vulkan, 
                           this->device, 
                           this->render_command.get(), 
                           this->extent);

    root->render(&renderer);

    FenceScope fence(this->device->device, this->render_fence->fence);

    std::vector<VkSemaphore> wait_semaphores{};
    std::vector<VkSemaphore> signal_semaphores = { this->rendering_finished->semaphore };

    this->render_command->submit(this->default_queue,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 this->render_fence->fence);
  }

  void present(Node * root)
  {
    root->present(this);
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  VkQueue default_queue{ nullptr };
  VkExtent2D extent;

  State state;

  std::shared_ptr<VulkanFence> render_fence;
  std::shared_ptr<VulkanFence> stage_fence;
  std::unique_ptr<VulkanCommandBuffers> command;
  std::unique_ptr<VulkanCommandBuffers> render_command;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;
  std::unique_ptr<VulkanSemaphore> rendering_finished;

  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;
  std::vector<alloc_callback> alloc_callbacks;

};
