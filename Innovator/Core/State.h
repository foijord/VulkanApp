#pragma once

#include <Innovator/Core/Vulkan/Wrapper.h>

#include <glm/mat4x4.hpp>
#include <gli/load.hpp>

#include <vector>

struct VulkanLayoutBinding {
  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
};

struct VulkanVertexAttributeDescription {
  uint32_t location;
  uint32_t binding;
  VkFormat format;
  uint32_t offset;
  uint32_t stride;
  VkVertexInputRate inputrate;
  VkBuffer buffer;
};

struct VulkanIndexBufferDescription {
  VkIndexType type;
  VkBuffer buffer;
  uint32_t count;
};

struct VulkanBufferDescription {
  VulkanLayoutBinding layout;
  VkDescriptorBufferInfo buffer;
};

struct VulkanTextureDescription {
  VulkanLayoutBinding layout;
  VkDescriptorImageInfo image;
};

struct VulkanShaderModuleDescription {
  VkShaderStageFlagBits stage;
  VkShaderModule module;
};

struct VulkanBufferDataDescription {
  VkDeviceSize stride;
  size_t size;
  void * data;
};

struct MemoryState {
  VkDescriptorBufferInfo buffer;
  VulkanBufferDataDescription bufferdata;
};

struct StageState {
  VkDescriptorBufferInfo buffer;
  VulkanBufferDataDescription bufferdata;
};

struct RenderState {
  explicit RenderState() :
    ViewMatrix(glm::mat4(1)),
    ProjMatrix(glm::mat4(1)),
    ModelMatrix(glm::mat4(1))
  {}

  glm::mat4 ViewMatrix;
  glm::mat4 ProjMatrix;
  glm::mat4 ModelMatrix;
};

struct RecordState {
  RecordState() : 
    rasterizationstate(defaultRasterizationState())
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
  
  VkDescriptorBufferInfo buffer;

  VulkanLayoutBinding layout_binding;
  VulkanBufferDataDescription bufferdata;
  VulkanTextureDescription texture_description;
  VulkanVertexAttributeDescription attribute_description;
  VkPipelineRasterizationStateCreateInfo rasterizationstate;

  std::vector<VulkanTextureDescription> textures;
  std::vector<VulkanIndexBufferDescription> indices;
  std::vector<VulkanShaderModuleDescription> shaders;
  std::vector<VulkanBufferDescription> buffer_descriptions;
  std::vector<VulkanVertexAttributeDescription> attribute_descriptions;
};
