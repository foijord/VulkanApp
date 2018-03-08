#pragma once

#include <Innovator/Core/Misc/Defines.h>

#include <memory>

class State;
class RenderAction;
class VulkanCommandBuffers;
class ComputePipelineObject;
class GraphicsPipelineObject;

class ComputeCommandObject {
public:
  NO_COPY_OR_ASSIGNMENT(ComputeCommandObject);
  ComputeCommandObject() = delete;
  ~ComputeCommandObject() = default;

  ComputeCommandObject(RenderAction * action, State & state);

  std::unique_ptr<ComputePipelineObject> pipeline;
  std::unique_ptr<VulkanCommandBuffers> command;
};

class DrawCommandObject {
public:
  NO_COPY_OR_ASSIGNMENT(DrawCommandObject);
  DrawCommandObject() = default;
  virtual ~DrawCommandObject() = default;

  DrawCommandObject(RenderAction * action, State & state);

  std::unique_ptr<GraphicsPipelineObject> pipeline;
  std::unique_ptr<VulkanCommandBuffers> command;
};

