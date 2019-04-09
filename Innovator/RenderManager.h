#pragma once

#include <Innovator/Misc/Defines.h>
#include <Innovator/Node.h>
#include <Innovator/State.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>

template <typename NodeType> void
SearchAction(const std::shared_ptr<Node> & root, std::vector<std::shared_ptr<NodeType>> & results) {
  auto group = std::dynamic_pointer_cast<Group>(root);
  if (group) {
    for (auto & node : group->children) {
      SearchAction<NodeType>(node, results);
    }
  } else {
    auto result = std::dynamic_pointer_cast<NodeType>(root);
    if (result) {
      results.push_back(result);
    }
  }
}

class MemoryAllocator {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryAllocator)
  MemoryAllocator() = delete;

  explicit MemoryAllocator(std::shared_ptr<VulkanDevice> device,
                           VkExtent2D extent) :
    device(std::move(device)),
    extent(extent)
  {}

  ~MemoryAllocator()
  {
    try {
      this->allocate();
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  MemoryState state{};
  std::shared_ptr<VulkanDevice> device;
  VkExtent2D extent;
  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;

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

class MemoryStager {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryStager)
  MemoryStager() = delete;
  ~MemoryStager() = default;

  explicit MemoryStager(std::shared_ptr<VulkanInstance> vulkan, 
                        std::shared_ptr<VulkanDevice> device,
                        VkCommandBuffer command,
                        VkExtent2D extent) :
    vulkan(std::move(vulkan)),
    device(std::move(device)),
    command(command),
    extent(extent),
    command_scope(std::make_unique<VulkanCommandBufferScope>(command))
  {}

  StageState state{};
  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  VkCommandBuffer command;
  VkExtent2D extent;
  std::unique_ptr<VulkanCommandBufferScope> command_scope;
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
    command(command)
  {
    state.extent = extent;
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  RenderState state;
  VulkanCommandBuffers * command;
};

class RenderManager {
public:
  NO_COPY_OR_ASSIGNMENT(RenderManager)
  RenderManager() = delete;

  explicit RenderManager(std::shared_ptr<VulkanInstance> vulkan,
                         std::shared_ptr<VulkanDevice> device) :
      vulkan(vulkan),
      device(std::move(device)), 
      render_fence(std::make_unique<VulkanFence>(this->device)),
      stage_fence(std::make_unique<VulkanFence>(this->device)),
      render_command(std::make_unique<VulkanCommandBuffers>(this->device)),
      pipelinecache(std::make_shared<VulkanPipelineCache>(this->device)),
      rendering_finished(std::make_unique<VulkanSemaphore>(this->device))
  {}

  virtual ~RenderManager() 
  {
    try {
      THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    } 
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void resize(VkExtent2D extent)
  {
    this->extent = extent;
  }

  void alloc(Node * root) const
  {
    MemoryAllocator allocator(this->device, this->extent);
    root->alloc(&allocator);
  }

  void stage(Node * root) const
  {
    VulkanCommandBuffers staging_command(this->device);
    {
      MemoryStager stager(this->vulkan, 
                          this->device, 
                          staging_command.buffer(), 
                          this->extent);

      root->stage(&stager);
    }

    FenceScope fence_scope(this->device->device, this->stage_fence->fence);
    
    staging_command.submit(this->device->default_queue,
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

    this->render_command->submit(this->device->default_queue,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 this->render_fence->fence);
  }

  void present(Node * root) const
  {
    SceneRenderer renderer(this->vulkan, 
                           this->device, 
                           this->render_command.get(), 
                           this->extent);

    root->present(&renderer);
  }

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanFence> render_fence;
  std::shared_ptr<VulkanFence> stage_fence;
  std::unique_ptr<VulkanCommandBuffers> render_command;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;
  std::unique_ptr<VulkanSemaphore> rendering_finished;

  VkExtent2D extent;
};
