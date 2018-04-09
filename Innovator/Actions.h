#pragma once

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
  ~MemoryAllocator() = default;

  explicit MemoryAllocator(std::shared_ptr<VulkanDevice> device) :
    device(std::move(device))
  {}

  MemoryState state;
  std::shared_ptr<VulkanDevice> device;
  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;
};

class MemoryStager {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryStager)
  MemoryStager() = delete;

  explicit MemoryStager(std::shared_ptr<VulkanDevice> device,
                        VulkanCommandBuffers * command) :
    device(std::move(device)),
    command(command)
  {
    this->command->begin();
  }

  ~MemoryStager()
  {
    try {
      this->command->end();
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  StageState state;
  std::shared_ptr<VulkanDevice> device;
  VulkanCommandBuffers * command;
};

class CommandRecorder {
public:
  NO_COPY_OR_ASSIGNMENT(CommandRecorder)
  CommandRecorder() = delete;

  explicit CommandRecorder(std::shared_ptr<VulkanDevice> device,
                           std::shared_ptr<VulkanRenderpass> renderpass,
                           std::shared_ptr<VulkanPipelineCache> pipelinecache,
                           VkFramebuffer framebuffer,
                           VkRect2D scissor,
                           VkViewport viewport,
                           VkRect2D renderarea,
                           const std::vector<VkClearValue> & clearvalues,
                           VulkanCommandBuffers * command) :
    device(std::move(device)),
    renderpass(std::move(renderpass)),
    pipelinecache(std::move(pipelinecache)),
    command(command)
  {
    this->command->begin();

    VkRenderPassBeginInfo begin_info{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,        // sType
      nullptr,                                         // pNext
      this->renderpass->renderpass,                    // renderPass
      framebuffer,                                     // framebuffer
      renderarea,                                      // renderArea
      static_cast<uint32_t>(clearvalues.size()),       // clearValueCount
      clearvalues.data()                               // pClearValues
    };

    vkCmdBeginRenderPass(this->command->buffer(), &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(this->command->buffer(), 0, 1, &viewport);
    vkCmdSetScissor(this->command->buffer(), 0, 1, &scissor);
  }

  ~CommandRecorder()
  {
    vkCmdEndRenderPass(this->command->buffer());

    try {
      this->command->end();
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  RecordState state;

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;

  VulkanCommandBuffers * command;
};

class SceneRenderer {
public:
  NO_COPY_OR_ASSIGNMENT(SceneRenderer)
   SceneRenderer() = delete;
  ~SceneRenderer() = default;

  explicit SceneRenderer(VkViewport viewport) :
    viewport(viewport)
  {}

  VkViewport viewport;
  RenderState state;
};

class SceneManager {
public:
  NO_COPY_OR_ASSIGNMENT(SceneManager)
  SceneManager() = delete;

  explicit SceneManager(std::shared_ptr<VulkanDevice> device,
                        std::shared_ptr<VulkanRenderpass> renderpass,
                        std::shared_ptr<VulkanFramebuffer> framebuffer,
                        const VkExtent2D extent)
    : device(std::move(device)), 
      renderpass(std::move(renderpass)),
      framebuffer(std::move(framebuffer)),
      extent(extent),
      stage_fence(std::make_unique<VulkanFence>(this->device)),
      render_fence(std::make_unique<VulkanFence>(this->device)),
      render_command(std::make_unique<VulkanCommandBuffers>(this->device)),
      staging_command(std::make_unique<VulkanCommandBuffers>(this->device)),
      pipelinecache(std::make_shared<VulkanPipelineCache>(this->device))
  {
    this->scissor = VkRect2D{{ 0, 0 }, this->extent};

    this->renderarea = { 
      { 0, 0 },             // offset
      this->extent          // extent
    };

    this->clearvalues = {
      {{{0.0f, 0.0f, 0.0f, 0.0f}}},
      {{{1.0f, 0}}}
    };

    this->viewport = { 
      0.0f,                                     // x
      0.0f,                                     // y
      static_cast<float>(this->extent.width),   // width
      static_cast<float>(this->extent.height),  // height
      0.0f,                                     // minDepth
      1.0f                                      // maxDepth
    };
  }

  virtual ~SceneManager() 
  {
    try {
      THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    } catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void alloc(const std::shared_ptr<Node> & root) const
  {
    MemoryAllocator allocator(this->device);
    root->alloc(&allocator);

    for (auto & image : allocator.imageobjects) {
      image->bind();
    }

    for (auto & buffer_object : allocator.bufferobjects) {
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

      auto memory_type_index = this->device->physical_device.getMemoryTypeIndex(
        memory_requirements,
        buffer_object->memory_property_flags);

      const auto memory = std::make_shared<VulkanMemory>(
        this->device,
        memory_requirements.size,
        memory_type_index);

      const VkDeviceSize offset = 0;

      buffer_object->bind(buffer, memory, offset);
    }
  }

  void stage(const std::shared_ptr<Node> & root) const
  {
    {
      MemoryStager stager(this->device, this->staging_command.get());
      root->stage(&stager);
    }

    this->stage_fence->reset();

    this->staging_command->submit(this->device->default_queue,
                                  VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                  this->stage_fence->fence);

    this->stage_fence->wait();
  }

  void record(const std::shared_ptr<Node> & root) const
  {
    CommandRecorder recorder(this->device,
                             this->renderpass,
                             this->pipelinecache,
                             this->framebuffer->framebuffer,
                             this->scissor,
                             this->viewport,
                             this->renderarea,
                             this->clearvalues,
                             this->render_command.get());

    root->record(&recorder);
  }

  void submit(const std::shared_ptr<Node> & root) const
  {
    SceneRenderer renderer(this->viewport);
    root->render(&renderer);

    this->render_fence->reset();

    this->render_command->submit(this->device->default_queue,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 this->render_fence->fence);

    this->render_fence->wait();
  }

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanFramebuffer> framebuffer;

  VkExtent2D extent{};
  VkRect2D scissor{};

  VkViewport viewport{};
  VkRect2D renderarea{};

  std::vector<VkClearValue> clearvalues;

  std::shared_ptr<VulkanFence> stage_fence;
  std::shared_ptr<VulkanFence> render_fence;

  std::unique_ptr<VulkanCommandBuffers> render_command;
  std::unique_ptr<VulkanCommandBuffers> staging_command;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;
};
