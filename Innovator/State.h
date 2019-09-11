#pragma once

#include <Innovator/Wrapper.h>

#include <glm/glm.hpp>
#include <vector>

struct VulkanIndexBufferDescription {
  VkIndexType type;
  VkBuffer buffer{ nullptr };
};

struct State {
  VkDescriptorBufferInfo descriptor_buffer_info{
    nullptr, 0, 0
  };
  VkBuffer buffer{ nullptr };
  class BufferData * bufferdata{ nullptr };
  class VulkanTextureImage* texture{ nullptr };
  std::shared_ptr<VulkanRenderpass> renderpass{ nullptr };
  VkExtent2D extent{ 0, 0 };

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

  VkImage image{ nullptr };
  VkImageView imageView { nullptr };
  VkImageLayout imageLayout { VK_IMAGE_LAYOUT_UNDEFINED };
  VkSampler sampler{ nullptr };

  std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
  std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
  std::vector<VkWriteDescriptorSet> write_descriptor_sets;
  std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
  std::vector<VkPushConstantRange> push_constant_ranges;
  std::vector<VkVertexInputBindingDescription> vertex_input_bindings;
  std::vector<VkVertexInputAttributeDescription> vertex_attributes;

  VulkanIndexBufferDescription index_buffer_description;
  std::vector<VkBuffer> vertex_attribute_buffers;
  std::vector<VkDeviceSize> vertex_attribute_buffer_offsets;
};

struct RenderState {
  glm::dmat4 ModelMatrix{ 1.0 };
  glm::dmat4 ViewMatrix{ 1.0 };
  glm::dmat4 ProjMatrix{ 1.0 };
};
