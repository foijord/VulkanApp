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

  void allocate()
  {
    for (auto & image : this->imageobjects) {
      image->bind();
    }

    for (auto & buffer_object : this->bufferobjects) {
      const auto buffer = std::make_shared<VulkanBuffer>(
        this->device,
        buffer_object->flags,
        buffer_object->size,
        buffer_object->usage,
        buffer_object->sharingMode);

      VkMemoryRequirements memory_requirements;
      vkGetBufferMemoryRequirements(
        buffer->device->device,
        buffer->buffer,
        &memory_requirements);

      auto memory_type_index = this->device->physical_device.getMemoryTypeIndex(
        memory_requirements,
        buffer_object->memory_property_flags);

      const auto memory = std::make_shared<VulkanMemory>(
        this->device,
        memory_requirements.size,
        memory_type_index);

      const VkDeviceSize memory_offset = 0;
      const VkDeviceSize buffer_offset = 0;

      buffer_object->bind(buffer, memory, buffer_offset, memory_offset);
    }
  }

  State state;
  std::shared_ptr<VulkanDevice> device;
  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;
};

class MemoryStager {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryStager)
  MemoryStager() = delete;

  explicit MemoryStager(std::shared_ptr<VulkanDevice> device,
                        std::shared_ptr<VulkanFence> fence,
                        const std::shared_ptr<Node> & root) :
    device(std::move(device)),
    fence(std::move(fence)),
    command(std::make_unique<VulkanCommandBuffers>(this->device))
  {
    THROW_ON_ERROR(vkWaitForFences(this->device->device, 1, &this->fence->fence, VK_TRUE, UINT64_MAX));

    this->command->begin();

    root->stage(this);

    this->command->end();

    THROW_ON_ERROR(vkResetFences(this->device->device, 1, &this->fence->fence));

    this->command->submit(
      this->device->default_queue,
      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      {},
      {},
      this->fence->fence);
  }

  ~MemoryStager()
  {
    try {
      THROW_ON_ERROR(vkWaitForFences(this->device->device, 1, &this->fence->fence, VK_TRUE, UINT64_MAX));
    }
    catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  State state;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanFence> fence;
  std::unique_ptr<VulkanCommandBuffers> command;
};

class SceneRenderer {
public:
  NO_COPY_OR_ASSIGNMENT(SceneRenderer)
  SceneRenderer() = delete;
  ~SceneRenderer() = default;

  explicit SceneRenderer(VkViewport viewport) :
    state(viewport)
  {}

  RenderState state;
};

class SceneManager {
public:
  NO_COPY_OR_ASSIGNMENT(SceneManager)
  SceneManager() = delete;

  explicit SceneManager(std::shared_ptr<VulkanDevice> device,
                        std::shared_ptr<VulkanRenderpass> renderpass,
                        std::shared_ptr<VulkanFramebuffer> framebuffer, 
                        VkExtent2D extent)
    : device(std::move(device)), 
      renderpass(std::move(renderpass)),
      framebuffer(std::move(framebuffer)),
      extent(extent),
      fence(std::make_unique<VulkanFence>(this->device)),
      command(std::make_unique<VulkanCommandBuffers>(this->device)),
      pipelinecache(std::make_unique<VulkanPipelineCache>(this->device))
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
    allocator.allocate();
  }

  void stage(const std::shared_ptr<Node> & root) const
  {
    MemoryStager stager(this->device, this->fence, root);
  }

  void record(const std::shared_ptr<Node> & root)
  {
    THROW_ON_ERROR(vkWaitForFences(this->device->device, 1, &this->fence->fence, VK_TRUE, UINT64_MAX));

    this->command->begin();

    VkRenderPassBeginInfo begin_info{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,        // sType
      nullptr,                                         // pNext
      this->renderpass->renderpass,                    // renderPass
      this->framebuffer->framebuffer,                  // framebuffer
      this->renderarea,                                // renderArea
      static_cast<uint32_t>(this->clearvalues.size()), // clearValueCount
      this->clearvalues.data()                         // pClearValues
    };

    vkCmdBeginRenderPass(this->command->buffer(), &begin_info, VK_SUBPASS_CONTENTS_INLINE);

    root->record(this);

    vkCmdEndRenderPass(this->command->buffer());

    this->command->end();
  }

  void submit(const std::shared_ptr<Node> & root) const
  {
    SceneRenderer renderer(this->viewport);
    root->render(&renderer);

    THROW_ON_ERROR(vkWaitForFences(this->device->device, 1, &this->fence->fence, VK_TRUE, UINT64_MAX));
    THROW_ON_ERROR(vkResetFences(this->device->device, 1, &this->fence->fence));

    this->command->submit(this->device->default_queue, 
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 
                          {}, 
                          {}, 
                          this->fence->fence);
  }

  State state;

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanFramebuffer> framebuffer;

  VkExtent2D extent{};
  VkRect2D scissor{};

  VkViewport viewport{};
  VkRect2D renderarea{};

  std::vector<VkClearValue> clearvalues;

  std::shared_ptr<VulkanFence> fence;
  std::unique_ptr<VulkanCommandBuffers> command;
  std::unique_ptr<VulkanPipelineCache> pipelinecache;
};
