#pragma once

#include <Innovator/Core/Misc/Defines.h>

#include <memory>

class State;
class SceneManager;
class VulkanCommandBuffers;
class ComputePipelineObject;
class GraphicsPipelineObject;

class ComputeCommandObject {
public:
  NO_COPY_OR_ASSIGNMENT(ComputeCommandObject);
  ComputeCommandObject() = delete;
  ~ComputeCommandObject() = default;

  ComputeCommandObject(SceneManager * action, State & state);

  std::unique_ptr<ComputePipelineObject> pipeline;
  std::unique_ptr<VulkanCommandBuffers> command;
};

class DrawCommandObject {
public:
  NO_COPY_OR_ASSIGNMENT(DrawCommandObject);
  DrawCommandObject() = default;
  virtual ~DrawCommandObject() = default;

  DrawCommandObject(SceneManager * action, State & state);

  std::unique_ptr<GraphicsPipelineObject> pipeline;
  std::unique_ptr<VulkanCommandBuffers> command;
};

