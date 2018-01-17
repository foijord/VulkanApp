#pragma once

#include <Innovator/State.h>
#include <Innovator/Node.h>
#include <Innovator/core/Math/Box.h>
#include <Innovator/core/VulkanObjects.h>

#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <string>

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
  RenderAction(const std::shared_ptr<Device> & device, VkFormat format, VkExtent2D extent)
    : device(device), extent(extent)
  {
    this->render_command = std::make_unique<VulkanCommandBuffers>(this->device->device, this->device->command_pool);
    this->staging_command = std::make_unique<VulkanCommandBuffers>(this->device->device, this->device->command_pool);
    this->pipeline_cache = std::make_unique<VulkanPipelineCache>(this->device->device);

    this->staging_command->begin();
    this->framebuffer_object = std::make_unique<FramebufferObject>(this->device->device, this->staging_command->buffer(), format, extent);
    this->staging_command->end();
    this->staging_command->submit(this->device->queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    THROW_ERROR(vkQueueWaitIdle(this->device->queue));

    this->viewport = { 0.0f, 0.0f, (float)this->extent.width, (float)this->extent.height, 0.0f, 1.0f };
    this->scissor = { { 0, 0 },  this->extent };
  }


  ~RenderAction() {}

  void apply(const std::shared_ptr<Node> & root)
  {
    this->staging_command->begin();
    root->traverse(this);
    this->staging_command->end();
    this->staging_command->submit(this->device->queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    THROW_ERROR(vkQueueWaitIdle(this->device->queue));

    this->render_command->begin();

    for (State & state : this->compute_states) {
      vkCmdExecuteCommands(this->render_command->buffer(), static_cast<uint32_t>(state.command->buffers.size()), state.command->buffers.data());
    }
    this->framebuffer_object->begin(this->render_command->buffer());

    for (State & state : this->graphic_states) {
      vkCmdExecuteCommands(this->render_command->buffer(), static_cast<uint32_t>(state.command->buffers.size()), state.command->buffers.data());
    }
    this->framebuffer_object->end(this->render_command->buffer());
    this->render_command->end();
    this->render_command->submit(this->device->queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    THROW_ERROR(vkQueueWaitIdle(this->device->queue));
    this->graphic_states.clear();
    this->compute_states.clear();
  }

  VkExtent2D extent;
  VkRect2D scissor;
  VkViewport viewport;

  std::vector<State> graphic_states;
  std::vector<State> compute_states;

  std::shared_ptr<Device> device;
  std::unique_ptr<VulkanCommandBuffers> render_command;
  std::unique_ptr<VulkanCommandBuffers> staging_command;
  std::unique_ptr<VulkanPipelineCache> pipeline_cache;
  std::unique_ptr<FramebufferObject> framebuffer_object;
};

class StateScope {
public:
  StateScope(Action * action) : state(action->state), action(action) {}
  ~StateScope() { action->state = state; }
private:
  State state;
  Action * action;
};
