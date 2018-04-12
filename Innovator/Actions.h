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

  explicit MemoryStager(std::shared_ptr<VulkanDevice> device) :
    device(std::move(device)),
    command(std::make_unique<VulkanCommandBuffers>(this->device)),
    fence(std::make_unique<VulkanFence>(this->device))
  {
    this->command->begin();
  }

  ~MemoryStager()
  {
    try {
      this->command->end();
      this->fence->reset();

      this->command->submit(this->device->default_queue,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            this->fence->fence);

      this->fence->wait();
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  StageState state{};
  std::shared_ptr<VulkanDevice> device;
  std::unique_ptr<VulkanCommandBuffers> command;
  std::unique_ptr<VulkanFence> fence;
};

class CommandRecorder {
public:
  NO_COPY_OR_ASSIGNMENT(CommandRecorder)
  CommandRecorder() = delete;

  explicit CommandRecorder(std::shared_ptr<VulkanDevice> device,
                           std::shared_ptr<VulkanRenderpass> renderpass,
                           std::shared_ptr<VulkanFramebuffer> framebuffer,
                           std::shared_ptr<VulkanPipelineCache> pipelinecache,
                           VulkanCommandBuffers * command) :
    device(std::move(device)),
    renderpass(std::move(renderpass)),
    framebuffer(std::move(framebuffer)),
    pipelinecache(std::move(pipelinecache)),
    command(command)
  {
    //this->command->begin();
  }

  ~CommandRecorder()
  {
    //try {
    //  //this->command->end();
    //}
    //catch (std::exception & e) {
    //  std::cerr << e.what() << std::endl;
    //}
  }

  RecordState state;

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanFramebuffer> framebuffer;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;

  VulkanCommandBuffers * command;
};

class SceneRenderer {
public:
  NO_COPY_OR_ASSIGNMENT(SceneRenderer)
  SceneRenderer(VulkanCommandBuffers * command) :
    command(command)
  {}

  ~SceneRenderer() = default;

  RenderState state;
  VulkanCommandBuffers * command;
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

  void alloc(const std::shared_ptr<Node> & root) const
  {
    MemoryAllocator allocator(device);
    root->alloc(&allocator);
  }

  void stage(const std::shared_ptr<Node> & root) const
  {
    MemoryStager stager(device);
    root->stage(&stager);
  }

  void record(const std::shared_ptr<Node> & root,
              std::shared_ptr<VulkanFramebuffer> framebuffer,
              const VkExtent2D extent) const
  {
    CommandRecorder recorder(this->device,
                             this->renderpass,
                             framebuffer,
                             this->pipelinecache,
                             this->render_command.get());

    root->record(&recorder);
  }

  void submit(const std::shared_ptr<Node> & root,
              std::shared_ptr<VulkanFramebuffer> framebuffer,
              const VkExtent2D extent) const
  {
    VkRect2D scissor{ { 0, 0 }, extent };

    const VkRect2D renderarea{
      { 0, 0 },             // offset
      extent                // extent
    };

    std::vector<VkClearValue> clearvalues{
      { { { 0.0f, 0.0f, 0.0f, 0.0f } } },
    { { { 1.0f, 0 } } }
    };

    VkViewport viewport{
      0.0f,                                     // x
      0.0f,                                     // y
      static_cast<float>(extent.width),         // width
      static_cast<float>(extent.height),        // height
      0.0f,                                     // minDepth
      1.0f                                      // maxDepth
    };

    VkRenderPassBeginInfo begin_info{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,        // sType
      nullptr,                                         // pNext
      this->renderpass->renderpass,                    // renderPass
      framebuffer->framebuffer,                        // framebuffer
      renderarea,                                      // renderArea
      static_cast<uint32_t>(clearvalues.size()),       // clearValueCount
      clearvalues.data()                               // pClearValues
    };

    this->render_command->begin();

    vkCmdBeginRenderPass(this->render_command->buffer(), &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(this->render_command->buffer(), 0, 1, &viewport);
    vkCmdSetScissor(this->render_command->buffer(), 0, 1, &scissor);

    SceneRenderer renderer(this->render_command.get());
    root->render(&renderer);

    vkCmdEndRenderPass(this->render_command->buffer());
    this->render_command->end();

    this->render_fence->reset();

    this->render_command->submit(this->device->default_queue,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 this->render_fence->fence);

    this->render_fence->wait();
  }

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;

  std::shared_ptr<VulkanFence> render_fence;
  std::unique_ptr<VulkanCommandBuffers> render_command;
  std::shared_ptr<VulkanPipelineCache> pipelinecache;
};
