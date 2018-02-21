#pragma once

#include <Innovator/State.h>
#include <Innovator/Node.h>
#include <Innovator/core/Math/Box.h>
#include <Innovator/core/VulkanObjects.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <string>
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
      this->descriptor_set = std::make_unique<DescriptorSetObject>(action->device, state.textures, state.buffers);

      this->pipeline = std::make_unique<GraphicsPipelineObject>(
        action->device,
        state.attributes,
        state.shaders,
        state.rasterization_state,
        this->descriptor_set->pipeline_layout->layout,
        action->renderpass->renderpass,
        action->pipeline_cache->cache,
        state.draw_description.topology);

      this->command = std::make_unique<VulkanCommandBuffers>(action->device, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      this->command->begin(0, action->renderpass->renderpass, 0, action->framebuffer->framebuffer, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

      this->descriptor_set->bind(this->command->buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS);
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
        vkCmdDraw(this->command->buffer(), state.draw_description.count, 1, 0, 0);
      }
      this->command->end();
    }
    ~DrawCommandObject() {}

    std::unique_ptr<class DescriptorSetObject> descriptor_set;
    std::unique_ptr<class GraphicsPipelineObject> pipeline;
    std::unique_ptr<VulkanCommandBuffers> command;
  };

  class ComputeCommandObject {
  public:
    ComputeCommandObject(RenderAction * action, State & state)
    {
      this->descriptor_set = std::make_unique<DescriptorSetObject>(action->device, state.textures, state.buffers);

      VkPipelineShaderStageCreateInfo shader_stage_info;
      shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shader_stage_info.pNext = nullptr;
      shader_stage_info.flags = 0; // reserved for future use
      shader_stage_info.stage = state.shaders[0].stage;
      shader_stage_info.module = state.shaders[0].module;
      shader_stage_info.pName = "main";
      shader_stage_info.pSpecializationInfo = nullptr;

      this->pipeline = std::make_unique<VulkanComputePipeline>(
        action->device,
        action->pipeline_cache->cache,
        0,
        shader_stage_info,
        this->descriptor_set->pipeline_layout->layout);

      this->command = std::make_unique<VulkanCommandBuffers>(action->device, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
      this->command->begin();

      this->pipeline->bind(this->command->buffer());
      this->descriptor_set->bind(this->command->buffers[0], VK_PIPELINE_BIND_POINT_COMPUTE);

      vkCmdDispatch(this->command->buffer(), 
        state.compute_description.group_count_x, 
        state.compute_description.group_count_y, 
        state.compute_description.group_count_z);

      this->command->end();
    }

    ~ComputeCommandObject() {}

    std::unique_ptr<class DescriptorSetObject> descriptor_set;
    std::unique_ptr<class VulkanComputePipeline> pipeline;
    std::unique_ptr<VulkanCommandBuffers> command;
  };

  RenderAction(const std::shared_ptr<VulkanDevice> & device, 
               const std::shared_ptr<VulkanFramebuffer> & framebuffer, 
               const std::shared_ptr<VulkanRenderpass> & renderpass, 
               VkExtent2D extent)
    : device(device), 
      framebuffer(framebuffer),
      renderpass(renderpass),
      extent(extent),
      command(std::make_unique<VulkanCommandBuffers>(device)),
      pipeline_cache(std::make_unique<VulkanPipelineCache>(device))
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
    this->command->begin();
    root->traverse(this);

    for (State & state : this->compute_states) {
      if (this->compute_commands.find(&state.compute_description) == this->compute_commands.end()) {
        this->compute_commands[&state.compute_description] = std::make_unique<ComputeCommandObject>(this, state);
      }
      VulkanCommandBuffers * command = this->compute_commands[&state.compute_description]->command.get();
      vkCmdExecuteCommands(this->command->buffer(), static_cast<uint32_t>(command->buffers.size()), command->buffers.data());
    }

    VkRenderPassBeginInfo begin_info = {
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
      if (this->draw_commands.find(&state.draw_description) == this->draw_commands.end()) {
        this->draw_commands[&state.draw_description] = std::make_unique<DrawCommandObject>(this, state);
      }
      VulkanCommandBuffers * command = this->draw_commands[&state.draw_description]->command.get();
      vkCmdExecuteCommands(this->command->buffer(), static_cast<uint32_t>(command->buffers.size()), command->buffers.data());
    }

    vkCmdEndRenderPass(this->command->buffer());
    this->command->end();

    this->command->submit(this->device->default_queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    THROW_ERROR(vkQueueWaitIdle(this->device->default_queue));
    this->graphic_states.clear();
    this->compute_states.clear();
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
  std::unique_ptr<VulkanCommandBuffers> command;
  std::unique_ptr<VulkanPipelineCache> pipeline_cache;

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
