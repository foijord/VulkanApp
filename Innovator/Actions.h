#pragma once

#include <Innovator/Camera.h>

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Node.h>
#include <Innovator/Core/State.h>
#include <Innovator/Core/VulkanObjects.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>

template <typename NodeType> 
static std::shared_ptr<NodeType> 
SearchAction(const std::shared_ptr<Node> & root) {
  auto group = std::dynamic_pointer_cast<Group>(root);
  if (!group) {
    return std::dynamic_pointer_cast<NodeType>(root);
  }
  for (auto & node : group->children) {
    auto result = SearchAction<NodeType>(node);
    if (result)
      return result;
  }
  return std::shared_ptr<NodeType>();
}

class MemoryAllocator {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryAllocator)
  MemoryAllocator() = delete;

  explicit MemoryAllocator(std::shared_ptr<VulkanDevice> device) :
    device(std::move(device))
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
  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;

private:
  void allocate()
  {
    for (auto & image : imageobjects) {
      image->bind();
    }

    for (auto & buffer_object : bufferobjects) {
      const auto buffer = std::make_shared<VulkanBuffer>(
        this->device,
        buffer_object->flags,
        buffer_object->size,
        buffer_object->usage,
        buffer_object->sharingMode);

      VkMemoryRequirements memory_requirements;
      vkGetBufferMemoryRequirements(
        this->device->device,
        buffer->buffer,
        &memory_requirements);

      auto memory_type_index = buffer->device->physical_device.getMemoryTypeIndex(
        memory_requirements.memoryTypeBits,
        buffer_object->memory_property_flags);

      const auto memory = std::make_shared<VulkanMemory>(
        this->device,
        memory_requirements.size,
        memory_type_index);

      const VkDeviceSize offset = 0;

      buffer_object->bind(buffer, memory, offset);
    }
  }
};

class MemoryStager {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryStager)
  MemoryStager() = delete;
  ~MemoryStager() = default;

  explicit MemoryStager(std::shared_ptr<VulkanDevice> device,
                        VkCommandBuffer command) :
    device(std::move(device)),
    command(command),
    command_scope(std::make_unique<VulkanCommandBufferScope>(command))
  {}

  StageState state{};
  std::shared_ptr<VulkanDevice> device;
  VkCommandBuffer command;
  std::unique_ptr<VulkanCommandBufferScope> command_scope;
  std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
};

class CommandRecorder {
public:
  NO_COPY_OR_ASSIGNMENT(CommandRecorder)
  CommandRecorder() = delete;
  ~CommandRecorder() = default;

  explicit CommandRecorder(std::shared_ptr<VulkanDevice> device,
                           VkRenderPass renderpass,
                           VkFramebuffer framebuffer,
                           VkPipelineCache pipelinecache,
                           VkExtent2D extent,
                           VkCommandBuffer command) :
    device(std::move(device)),
    renderpass(renderpass),
    framebuffer(framebuffer),
    pipelinecache(pipelinecache),
    extent(extent),
    command(command)
  {}

  RecordState state;

  std::shared_ptr<VulkanDevice> device;
  VkRenderPass renderpass;
  VkFramebuffer framebuffer;
  VkPipelineCache pipelinecache;
  VkExtent2D extent;
  VkCommandBuffer command;
};

class PipelineCreator {
public:
  NO_COPY_OR_ASSIGNMENT(PipelineCreator)
  PipelineCreator() = delete;
  ~PipelineCreator() = default;

  explicit PipelineCreator(std::shared_ptr<VulkanDevice> device, 
                           VkRenderPass renderpass,
                           VkPipelineCache pipelinecache,
                           std::shared_ptr<VulkanDescriptorPool> descriptor_pool) :
    device(std::move(device)),
    renderpass(renderpass),
    pipelinecache(pipelinecache),
    descriptor_pool(std::move(descriptor_pool))
  {}

  PipelineState state;
  std::shared_ptr<VulkanDevice> device;
  VkRenderPass renderpass;
  VkPipelineCache pipelinecache;
  std::shared_ptr<VulkanDescriptorPool> descriptor_pool;
};

class SceneRenderer {
public:
  NO_COPY_OR_ASSIGNMENT(SceneRenderer)
  SceneRenderer() = delete;
  ~SceneRenderer() = default;

  explicit SceneRenderer(VulkanCommandBuffers * command,
                         Camera * camera) :
    command(command),
    camera(camera)
  {}


  RenderState state;
  VulkanCommandBuffers * command;
  Camera * camera;
};

class SceneManager {
public:
  NO_COPY_OR_ASSIGNMENT(SceneManager)
  SceneManager() = delete;

  explicit SceneManager(std::shared_ptr<VulkanDevice> device,
                        std::shared_ptr<VulkanRenderpass> renderpass)
    : device(std::move(device)), 
      renderpass(std::move(renderpass)),
      render_fence(std::make_unique<VulkanFence>(this->device)),
      render_command(std::make_unique<VulkanCommandBuffers>(this->device)),
      pipelinecache(std::make_shared<VulkanPipelineCache>(this->device))
  {}

  virtual ~SceneManager() 
  {
    try {
      THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    } 
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void alloc(Node * root) const
  {
    MemoryAllocator allocator(device);
    root->alloc(&allocator);
  }

  void stage(Node * root)
  {
    VulkanCommandBuffers staging_command(this->device);
    {
      MemoryStager stager(this->device, staging_command.buffer());
      root->stage(&stager);

      this->descriptor_pool = std::make_unique<VulkanDescriptorPool>(
        this->device,
        stager.descriptor_pool_sizes);
    }

    const VulkanFence fence(this->device);
    FenceScope fence_scope(this->device->device, fence.fence);
    
    staging_command.submit(this->device->default_queue,
                           VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                           fence.fence);
  }

  void pipeline(Node * root) const
  {
    PipelineCreator creator(this->device,
                            this->renderpass->renderpass,
                            this->pipelinecache->cache,
                            this->descriptor_pool);
    root->pipeline(&creator);
  }

  void record(Node * root, const VkFramebuffer framebuffer, const VkExtent2D extent) const
  {
    CommandRecorder recorder(this->device,
                             this->renderpass->renderpass,
                             framebuffer,
                             this->pipelinecache->cache,
                             extent,
                             this->render_command->buffer());

    root->record(&recorder);
  }

  void render(Node * root, const VkFramebuffer framebuffer, Camera * camera, const VkExtent2D extent) const
  {
    camera->updateMatrices();

    const VkRect2D renderarea{
      { 0, 0 },             // offset
      extent                // extent
    };

    const std::vector<VkClearValue> clearvalues{
      { { { 0.0f, 0.0f, 0.0f, 0.0f } } },
      { { { 1.0f, 0 } } }
    };

    {
      VulkanCommandBufferScope commandbuffer(this->render_command->buffer());

      VulkanRenderPassScope renderpass(this->renderpass->renderpass,
                                       framebuffer,
                                       renderarea,
                                       clearvalues,
                                       this->render_command->buffer());

      SceneRenderer renderer(this->render_command.get(), camera);
      root->render(&renderer);
    }
    {
      FenceScope fence(this->device->device, this->render_fence->fence);

      this->render_command->submit(this->device->default_queue,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                   this->render_fence->fence);
    }
  }

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;

  std::shared_ptr<VulkanFence> render_fence;
  std::unique_ptr<VulkanCommandBuffers> render_command;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;
  std::shared_ptr<VulkanDescriptorPool> descriptor_pool;
};
