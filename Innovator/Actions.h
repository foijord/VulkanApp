#pragma once

#include <Innovator/Node.h>
#include <Innovator/core/State.h>
#include <Innovator/core/VulkanObjects.h>
#include <Innovator/core/Math/Box.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

class Action {
public:
  State state;
};

class HandleEventAction : public Action {
public:
  void apply(const std::shared_ptr<Node> & root)
  {
    root->traverse(this);
  }

  std::string key;
};

template <typename NodeType> 
static std::shared_ptr<NodeType> 
SearchAction(const std::shared_ptr<Node> & root) {
  std::shared_ptr<Group> group = dynamic_pointer_cast<Group>(root);
  if (!group) {
    return dynamic_pointer_cast<NodeType>(root);
  }
  for (const std::shared_ptr<Node> & node : group->children) {
    std::shared_ptr<NodeType> result = SearchAction<NodeType>(node);
    if (result)
      return result;
  }
  return std::shared_ptr<NodeType>();
}

class BoundingBoxAction : public Action {
public:
  void apply(const std::shared_ptr<Node> & root)
  {
    root->traverse(this);
  }

  void extendBy(const box3 & box)
  {
    this->bounding_box.extendBy(box);
  }


  box3 bounding_box;
};

class RenderAction : public Action {
public:
  class DrawCommandObject {
  public:
    DrawCommandObject(RenderAction * action, State & state)
    {
      this->pipeline = std::make_unique<GraphicsPipelineObject>(
        action->device,
        state.attributes,
        state.shaders,
        state.buffers,
        state.textures,
        state.rasterizationstate,
        action->renderpass,
        action->pipelinecache,
        state.drawdescription);

      this->command = std::make_unique<VulkanCommandBuffers>(action->device, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      this->command->begin(0, action->renderpass->renderpass, 0, action->framebuffer->framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

      this->pipeline->bind(this->command->buffer());

      for (const VulkanVertexAttributeDescription & attribute : state.attributes) {
        VkDeviceSize offsets[1] = { 0 };
        vkCmdBindVertexBuffers(this->command->buffer(), 0, 1, &attribute.buffer, &offsets[0]);
      }
      vkCmdSetViewport(this->command->buffer(), 0, 1, &action->viewport);
      vkCmdSetScissor(this->command->buffer(), 0, 1, &action->scissor);

      if (state.indices.size()) {
        for (const VulkanIndexBufferDescription & indexbuffer : state.indices) {
          vkCmdBindIndexBuffer(this->command->buffer(), indexbuffer.buffer, 0, indexbuffer.type);
          vkCmdDrawIndexed(this->command->buffer(), indexbuffer.count, 1, 0, 0, 1);
        }
      }
      else {
        vkCmdDraw(this->command->buffer(), state.drawdescription.count, 1, 0, 0);
      }
      this->command->end();
    }
    ~DrawCommandObject() {}

    std::unique_ptr<GraphicsPipelineObject> pipeline;
    std::unique_ptr<VulkanCommandBuffers> command;
  };

  class ComputeCommandObject {
  public:
    ComputeCommandObject(RenderAction * action, State & state)
    {
      this->pipeline = std::make_unique<ComputePipelineObject>(
        action->device,
        state.shaders[0],
        state.buffers,
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

    ~ComputeCommandObject() {}

    std::unique_ptr<ComputePipelineObject> pipeline;
    std::unique_ptr<VulkanCommandBuffers> command;
  };

  RenderAction(const std::shared_ptr<VulkanDevice> & device, 
               const std::shared_ptr<VulkanRenderpass> & renderpass, 
               const std::shared_ptr<VulkanFramebuffer> & framebuffer, 
               VkExtent2D extent)
    : device(device), 
      extent(extent),
      renderpass(renderpass),
      framebuffer(framebuffer),
      fence(std::make_unique<VulkanFence>(device)),
      command(std::make_unique<VulkanCommandBuffers>(device)),
      pipelinecache(std::make_unique<VulkanPipelineCache>(device))
  {
    this->scissor = { { 0, 0 },  this->extent };
    this->renderarea = { { 0, 0 }, this->extent };
    this->clearvalues = { { 0.0f, 0.0f, 0.0f, 0.0f },{ 1.0f, 0 } };
    this->viewport = { 0.0f, 0.0f, (float)this->extent.width, (float)this->extent.height, 0.0f, 1.0f };
  }

  ~RenderAction() 
  {
    try {
      THROW_ERROR(vkDeviceWaitIdle(this->device->device));
    } catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  void apply(const std::shared_ptr<Node> & root)
  {
    THROW_ERROR(vkWaitForFences(this->device->device, 1, &this->fence->fence, VK_TRUE, UINT64_MAX));

    this->command->begin();
    root->traverse(this);

    for (State & state : this->compute_states) {
      if (this->compute_commands.find(&state.compute_description) == this->compute_commands.end()) {
        this->compute_commands[&state.compute_description] = std::make_unique<ComputeCommandObject>(this, state);
      }
      VulkanCommandBuffers * command = this->compute_commands[&state.compute_description]->command.get();
      vkCmdExecuteCommands(this->command->buffer(), static_cast<uint32_t>(command->buffers.size()), command->buffers.data());
    }

    VkRenderPassBeginInfo begin_info {
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

    THROW_ERROR(vkResetFences(this->device->device, 1, &this->fence->fence));
    this->command->submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, {}, {}, this->fence->fence);

    this->graphic_states.clear();
    this->compute_states.clear();
  }

  void clearCache()
  {
    THROW_ERROR(vkDeviceWaitIdle(this->device->device));
    this->draw_commands.clear();
    this->compute_commands.clear();
  }

  VkExtent2D extent;
  VkRect2D scissor;
  VkViewport viewport;
  VkRect2D renderarea;
  std::vector<VkClearValue> clearvalues;

  std::vector<State> graphic_states;
  std::vector<State> compute_states;

  std::map<VulkanDrawDescription *, std::unique_ptr<DrawCommandObject>> draw_commands;
  std::map<VulkanComputeDescription *, std::unique_ptr<ComputeCommandObject>> compute_commands;

  std::shared_ptr<VulkanDevice> device;
  std::unique_ptr<VulkanFence> fence;
  std::unique_ptr<VulkanCommandBuffers> command;
  std::unique_ptr<VulkanPipelineCache> pipelinecache;

  std::shared_ptr<VulkanFramebuffer> framebuffer;
  std::shared_ptr<VulkanRenderpass> renderpass;
};

class StateScope {
public:
  StateScope(Action * action) : state(action->state), action(action) {}
  ~StateScope() { action->state = state; }
private:
  State state;
  Action * action;
};
