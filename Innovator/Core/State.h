#pragma once

#include <Innovator/Core/Vulkan/Wrapper.h>

#include <glm/mat4x4.hpp>
#include <gli/load.hpp>

#include <vector>

struct VulkanIndexBufferDescription {
  VkIndexType type;
  VkBuffer buffer;
  uint32_t count;
};

struct VulkanBufferDataDescription {
  VkDeviceSize stride;
  size_t size;
  void * data;
};

struct MemoryState {
  VkDescriptorBufferInfo descriptor_buffer_info;
  VulkanBufferDataDescription buffer_data_description;
};

struct StageState {
  VkDescriptorBufferInfo descriptor_buffer_info;
  VulkanBufferDataDescription buffer_data_description;
};

struct RenderState {
  explicit RenderState() :
    ModelMatrix(glm::mat4(1))
  {}

  glm::mat4 ModelMatrix;
};

struct RecordState {
  VulkanBufferDataDescription buffer_data_description;
  VkPipelineRasterizationStateCreateInfo rasterization_state;

  VkDescriptorBufferInfo descriptor_buffer_info{
    nullptr, 0, 0
  };

  VkDescriptorImageInfo descriptor_image_info{
    nullptr, nullptr, VK_IMAGE_LAYOUT_UNDEFINED
  };
  VkSampler sampler{ nullptr };

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
