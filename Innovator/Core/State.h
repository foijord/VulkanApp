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

struct VulkanIndexBufferDescription {
  VkIndexType type;
  VkBuffer buffer;
  uint32_t count;
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
  VkPipelineRasterizationStateCreateInfo rasterizationstate;

  VkSampler sampler;

  std::vector<VulkanIndexBufferDescription> indices;
  std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;

  std::vector<VkWriteDescriptorSet> write_descriptor_sets;
  std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
  std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;

  std::vector<VkVertexInputBindingDescription> vertex_input_bindings;
  std::vector<VkVertexInputAttributeDescription> vertex_attributes;
  std::vector<VkBuffer> vertex_attribute_buffers;
  std::vector<VkDeviceSize> vertex_attribute_buffer_offsets;

};
