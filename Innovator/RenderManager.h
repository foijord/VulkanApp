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

template <typename NodeType> void
FindAll(Node * root, std::vector<NodeType*> & results) {
  auto group = dynamic_cast<Group*>(root);
  if (group) {
    for (auto & node : group->children) {
      FindAll<NodeType>(node.get(), results);
    }
  } else {
    auto result = dynamic_cast<NodeType*>(root);
    if (result) {
      results.push_back(result);
    }
  }
}

class TraversalContext {
public:
  typedef std::function<void(TraversalContext *)> alloc_callback;
  NO_COPY_OR_ASSIGNMENT(TraversalContext)
  TraversalContext() = delete;

  explicit TraversalContext(std::shared_ptr<VulkanInstance> vulkan, 
                            std::shared_ptr<VulkanDevice> device,
                            VkQueue queue,
                            VkCommandBuffer command,
                            VkExtent2D extent) :
    vulkan(std::move(vulkan)),
    device(std::move(device)),
    queue(queue),
    extent(extent),
    command(command),
    command_scope(std::make_unique<VulkanCommandBufferScope>(command))
  {}

  ~TraversalContext()
  {
    try {
      this->allocate();
      for (auto & alloc_callback : this->alloc_callbacks) {
        alloc_callback(this);
      }
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  State state{};
  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  VkQueue queue;
  VkExtent2D extent;
  VkCommandBuffer command;
  std::unique_ptr<VulkanCommandBufferScope> command_scope;

  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;
  std::vector<alloc_callback> alloc_callbacks;

private:
  void allocate()
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
};

class PipelineCreator {
public:
  NO_COPY_OR_ASSIGNMENT(PipelineCreator)
  PipelineCreator() = delete;
  ~PipelineCreator() = default;

  explicit PipelineCreator(std::shared_ptr<VulkanDevice> device, 
                           VkPipelineCache pipelinecache) :
    device(std::move(device)),
    pipelinecache(pipelinecache)
  {}

  PipelineState state;
  std::shared_ptr<VulkanDevice> device;
  VkPipelineCache pipelinecache;
};

class CommandRecorder {
public:
  NO_COPY_OR_ASSIGNMENT(CommandRecorder)
  CommandRecorder() = delete;
  ~CommandRecorder() = default;

  explicit CommandRecorder(std::shared_ptr<VulkanDevice> device,
                           VkPipelineCache pipelinecache,
                           VkExtent2D extent,
                           VkCommandBuffer command) :
    device(std::move(device)),
    pipelinecache(pipelinecache),
    extent(extent),
    command(command)
  {}

  RecordState state;

  std::shared_ptr<VulkanDevice> device;
  VkPipelineCache pipelinecache;
  VkExtent2D extent;
  VkCommandBuffer command;
};

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
  NO_COPY_OR_ASSIGNMENT(RenderManager)
  RenderManager() = delete;

  explicit RenderManager(std::shared_ptr<VulkanInstance> vulkan,
                         std::shared_ptr<VulkanDevice> device,
                         std::shared_ptr<Group> root,
                         VkExtent2D extent) :
      vulkan(std::move(vulkan)),
      device(std::move(device)),
      root(std::move(root)),
      extent(extent),
      render_fence(std::make_unique<VulkanFence>(this->device)),
      stage_fence(std::make_unique<VulkanFence>(this->device)),
      staging_command(std::make_unique<VulkanCommandBuffers>(this->device)),
      render_command(std::make_unique<VulkanCommandBuffers>(this->device)),
      pipelinecache(std::make_shared<VulkanPipelineCache>(this->device)),
      rendering_finished(std::make_unique<VulkanSemaphore>(this->device))
  {
    this->default_queue = this->device->getQueue(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

    this->alloc(this->root.get());
    this->stage(this->root.get());
    this->pipeline(this->root.get());
    this->record(this->root.get());
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

  void redraw()
  {
    try {
      this->render(this->root.get());
      this->present(this->root.get());
    }
    catch (VkException &) {
      // recreate swapchain, try again next frame
      //this->resize();
    }
  }

  void resize(VkExtent2D extent)
  {
    // make sure all work submitted is done before we start recreating stuff
    THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));

    this->resize(this->root.get(), extent);
    this->record(this->root.get());

    this->redraw();
  }

  void alloc(Node * root) const
  {    
    TraversalContext context(this->vulkan,
                             this->device,
                             this->default_queue,
                             this->staging_command->buffer(),
                             this->extent);

    root->alloc(&context);
  }

  void resize(Node * root, VkExtent2D extent)
  {
    this->extent = extent;
    {
      TraversalContext context(this->vulkan,
                               this->device,
                               this->default_queue,
                               this->staging_command->buffer(),
                               this->extent);

      root->resize(&context);
    }
    FenceScope fence_scope(this->device->device, this->stage_fence->fence);
    
    this->staging_command->submit(this->default_queue,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  this->stage_fence->fence);
  }

  void stage(Node * root) const
  {
    {
      TraversalContext context(this->vulkan,
                               this->device,
                               this->default_queue,
                               this->staging_command->buffer(),
                               this->extent);

      root->stage(&context);
    }
    FenceScope fence_scope(this->device->device, this->stage_fence->fence);
    
    this->staging_command->submit(this->default_queue,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  this->stage_fence->fence);
  }

  void pipeline(Node * root) const
  {
    PipelineCreator creator(this->device,
                            this->pipelinecache->cache);

    root->pipeline(&creator);
  }

  void record(Node * root) const
  {
    CommandRecorder recorder(this->device,
                             this->pipelinecache->cache,
                             this->extent,
                             this->render_command->buffer());

    root->record(&recorder);
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

  void present(Node * root) const
  {
    TraversalContext context(this->vulkan,
                             this->device,
                             this->default_queue,
                             this->staging_command->buffer(),
                             this->extent);

    root->present(&context);
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<Group> root;
  VkQueue default_queue{ nullptr };
  VkExtent2D extent;

  std::shared_ptr<VulkanFence> render_fence;
  std::shared_ptr<VulkanFence> stage_fence;
  std::unique_ptr<VulkanCommandBuffers> staging_command;
  std::unique_ptr<VulkanCommandBuffers> render_command;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;
  std::unique_ptr<VulkanSemaphore> rendering_finished;
};
