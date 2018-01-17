#pragma once

#include <Innovator/core/VulkanObjects.h>

#include <glm/mat4x4.hpp>
#include <vector>
#include <memory>

class State {
public:
  State() 
    : command(nullptr), 
      ViewMatrix(glm::mat4(1)),
      ProjMatrix(glm::mat4(1)),
      ModelMatrix(glm::mat4(1)),
      rasterization_state(defaultRasterizationState())
  {}

  static VkPipelineRasterizationStateCreateInfo defaultRasterizationState()
  {
    VkPipelineRasterizationStateCreateInfo default_state;
    ::memset(&default_state, 0, sizeof(VkPipelineRasterizationStateCreateInfo));
    default_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    default_state.polygonMode = VK_POLYGON_MODE_FILL;
    default_state.cullMode = VK_CULL_MODE_BACK_BIT;
    default_state.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    default_state.lineWidth = 1.0f;
    return default_state;
  }
  
  glm::mat4 ViewMatrix;
  glm::mat4 ProjMatrix;
  glm::mat4 ModelMatrix;

  class VulkanCommandBuffers * command;
  VulkanLayoutBinding layout_binding;
  VulkanVertexAttributeDescription attribute_description;
  VkPipelineRasterizationStateCreateInfo rasterization_state;

  std::vector<VulkanBufferDescription> buffers;
  std::vector<VulkanTextureDescription> textures;
  std::vector<VulkanIndexBufferDescription> indices;
  std::vector<VulkanShaderModuleDescription> shaders;
  std::vector<VulkanVertexAttributeDescription> attributes;
};
