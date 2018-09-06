#pragma once

#include <Innovator/Core/Vulkan/Wrapper.h>
#include <Innovator/Core/Math/Matrix.h>

#include <vector>

using namespace Innovator::Core::Math;

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
  VkDescriptorBufferInfo descriptor_buffer_info{
    nullptr, 0, 0
  };
  VulkanBufferDataDescription buffer_data_description{
    0, 0, nullptr
  };
};

struct StageState {
  VkDescriptorBufferInfo descriptor_buffer_info{
    nullptr, 0, 0
  };
  VulkanBufferDataDescription buffer_data_description{
    0, 0, nullptr
  };
};

struct PipelineState {
  VkPipelineRasterizationStateCreateInfo rasterization_state{
    VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, // sType
    nullptr,                                                    // pNext
    0,                                                          // flags;
    VK_FALSE,                                                   // depthClampEnable
    VK_FALSE,                                                   // rasterizerDiscardEnable
    VK_POLYGON_MODE_FILL,                                       // polygonMode
    VK_CULL_MODE_BACK_BIT,                                      // cullMode
    VK_FRONT_FACE_COUNTER_CLOCKWISE,                            // frontFace
    VK_FALSE,                                                   // depthBiasEnable
    0.0f,                                                       // depthBiasConstantFactor
    0.0f,                                                       // depthBiasClamp
    0.0f,                                                       // depthBiasSlopeFactor
    1.0f,                                                       // lineWidth
  };

  VkDescriptorBufferInfo descriptor_buffer_info{
    nullptr, 0, 0
  };

  VkDescriptorImageInfo descriptor_image_info{
    nullptr, nullptr, VK_IMAGE_LAYOUT_UNDEFINED
  };

  VkSampler sampler{
    nullptr
  };

  VulkanBufferDataDescription buffer_data_description{
    0, 0, nullptr
  };
  
  std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
  std::vector<VkWriteDescriptorSet> write_descriptor_sets;
  std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
  std::vector<VkVertexInputBindingDescription> vertex_input_bindings;
  std::vector<VkVertexInputAttributeDescription> vertex_attributes;
};

struct RecordState {
  VulkanBufferDataDescription buffer_data_description{
    0, 0, nullptr
  };
  VkBuffer buffer{
    nullptr
  };
  std::vector<VulkanIndexBufferDescription> indices;
  std::vector<VkBuffer> vertex_attribute_buffers;
  std::vector<VkDeviceSize> vertex_attribute_buffer_offsets;
};

struct RenderState {
  mat4d ModelMatrix{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  };
};
