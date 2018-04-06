#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Node.h>
#include <Innovator/Core/State.h>
#include <Innovator/Core/VulkanObjects.h>
#include <Innovator/Commands.h>

#include <map>
#include <memory>
#include <utility>
#include <vector>
#include <fstream>
#include <iostream>

class Action {
public:
  NO_COPY_OR_ASSIGNMENT(Action);
  Action() = default;
  virtual ~Action() = default;

  State state;
};

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

class RenderAction : public Action {
public:
  NO_COPY_OR_ASSIGNMENT(RenderAction);
  RenderAction() = default;

  explicit RenderAction(std::shared_ptr<VulkanDevice> device,
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

  virtual ~RenderAction() 
  {
    try {
      THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    } catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void alloc(const std::shared_ptr<Node> & root)
  {
    root->alloc(this);

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

      uint32_t memory_type_index = this->device->physical_device.getMemoryTypeIndex(
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

  void stage(const std::shared_ptr<Node> & root)
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

  void record(const std::shared_ptr<Node> & root)
  {
    THROW_ON_ERROR(vkWaitForFences(this->device->device, 1, &this->fence->fence, VK_TRUE, UINT64_MAX));

    root->record(this);

    this->command->begin();

    for (State & state : this->compute_states) {
      if (this->compute_commands.find(&state.compute_description) == this->compute_commands.end()) {
        this->compute_commands[&state.compute_description] = std::make_unique<ComputeCommandObject>(this, state);
      }
      VulkanCommandBuffers * command = this->compute_commands[&state.compute_description]->command.get();
      vkCmdExecuteCommands(this->command->buffer(), static_cast<uint32_t>(command->buffers.size()), command->buffers.data());
    }

    VkRenderPassBeginInfo begin_info{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,        // sType
      nullptr,                                         // pNext
      this->renderpass->renderpass,                    // renderPass
      this->framebuffer->framebuffer,                  // framebuffer
      this->renderarea,                                // renderArea
      static_cast<uint32_t>(this->clearvalues.size()), // clearValueCount
      this->clearvalues.data()                         // pClearValues
    };

    vkCmdBeginRenderPass(this->command->buffer(), &begin_info, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    for (State & state : this->graphic_states) {
      if (this->draw_commands.find(&state.drawdescription) == this->draw_commands.end()) {
        this->draw_commands[&state.drawdescription] = std::make_unique<DrawCommandObject>(this, state);
      }
      VulkanCommandBuffers * command = this->draw_commands[&state.drawdescription]->command.get();
      vkCmdExecuteCommands(this->command->buffer(), static_cast<uint32_t>(command->buffers.size()), command->buffers.data());
    }

    vkCmdEndRenderPass(this->command->buffer());
    this->command->end();

    THROW_ON_ERROR(vkResetFences(this->device->device, 1, &this->fence->fence));
    this->command->submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {}, {}, this->fence->fence);

    this->graphic_states.clear();
    this->compute_states.clear();
  }

  void render(const std::shared_ptr<Node> & root)
  {
    RenderState state;
    state.viewport = this->viewport;
    root->render(state);

    THROW_ON_ERROR(vkWaitForFences(this->device->device, 1, &this->fence->fence, VK_TRUE, UINT64_MAX));
    THROW_ON_ERROR(vkResetFences(this->device->device, 1, &this->fence->fence));
    this->command->submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {}, {}, this->fence->fence);
  }

  void clearCache()
  {
    THROW_ON_ERROR(vkDeviceWaitIdle(this->device->device));
    this->draw_commands.clear();
    this->compute_commands.clear();
  }

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanRenderpass> renderpass;
  std::shared_ptr<VulkanFramebuffer> framebuffer;

  VkExtent2D extent{};
  VkRect2D scissor{};

  VkViewport viewport{};
  VkRect2D renderarea{};

  std::vector<VkClearValue> clearvalues;

  std::vector<State> graphic_states;
  std::vector<State> compute_states;

  std::map<VulkanDrawDescription *, std::unique_ptr<DrawCommandObject>> draw_commands;
  std::map<VulkanComputeDescription *, std::unique_ptr<ComputeCommandObject>> compute_commands;

  std::vector<std::shared_ptr<ImageObject>> imageobjects;
  std::vector<std::shared_ptr<BufferObject>> bufferobjects;

  std::unique_ptr<VulkanFence> fence;
  std::unique_ptr<VulkanCommandBuffers> command;
  std::unique_ptr<VulkanPipelineCache> pipelinecache;
};

inline ComputeCommandObject::ComputeCommandObject(RenderAction * action, State & state)
{
  this->pipeline = std::make_unique<ComputePipelineObject>(
    action->device,
    state.shaders[0],
    state.buffer_descriptions,
    state.textures,
    action->pipelinecache);

  this->command = std::make_unique<VulkanCommandBuffers>(action->device, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  this->command->begin();

  this->pipeline->bind(this->command->buffer());

  vkCmdDispatch(this->command->buffer(),
    state.compute_description.group_count_x,
    state.compute_description.group_count_y,
    state.compute_description.group_count_z);

  this->command->end();
}

inline DrawCommandObject::DrawCommandObject(RenderAction * action, State & state)
{
  this->pipeline = std::make_unique<GraphicsPipelineObject>(
    action->device,
    state.attribute_descriptions,
    state.shaders,
    state.buffer_descriptions,
    state.textures,
    state.rasterizationstate,
    action->renderpass,
    action->pipelinecache,
    state.drawdescription);

  this->command = std::make_unique<VulkanCommandBuffers>(action->device, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  this->command->begin(0, action->renderpass->renderpass, 0, action->framebuffer->framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

  this->pipeline->bind(this->command->buffer());

  for (const auto& attribute : state.attribute_descriptions) {
    VkDeviceSize offsets[1] = { 0 };
    vkCmdBindVertexBuffers(this->command->buffer(), 0, 1, &attribute.buffer, &offsets[0]);
  }
  vkCmdSetViewport(this->command->buffer(), 0, 1, &action->viewport);
  vkCmdSetScissor(this->command->buffer(), 0, 1, &action->scissor);

  if (!state.indices.empty()) {
    for (const auto& indexbuffer : state.indices) {
      vkCmdBindIndexBuffer(this->command->buffer(), indexbuffer.buffer, 0, indexbuffer.type);
      vkCmdDrawIndexed(this->command->buffer(), indexbuffer.count, 1, 0, 0, 1);
    }
  }
  else {
    vkCmdDraw(this->command->buffer(), state.drawdescription.count, 1, 0, 0);
  }
  this->command->end();
}

class StateScope {
public:
  NO_COPY_OR_ASSIGNMENT(StateScope);
  
  explicit StateScope(Action * action) : 
    state(action->state), 
    action(action) 
  {}

  ~StateScope()
  {
    action->state = this->state;
  }

private:
  Action * action;
  State state;
};
