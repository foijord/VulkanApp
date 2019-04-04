#pragma once

#include <Innovator/Vulkan/Wrapper.h>
#include <Innovator/Math/Matrix.h>

#include <vector>

using namespace Innovator::Math;

struct VulkanIndexBufferDescription {
  VkIndexType type;
  VkBuffer buffer;
};

struct MemoryState {
  VkDescriptorBufferInfo descriptor_buffer_info{
    nullptr, 0, 0
  };
  class BufferData * bufferdata{ nullptr };
  VkExtent2D extent;
};

struct StageState {
  VkBuffer buffer{ nullptr };
  class BufferData * bufferdata{ nullptr };
  std::vector<VkImageView> framebuffer_attachments;
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

  VkBuffer buffer { nullptr };
  VkImageView imageView { nullptr };
  VkImageLayout imageLayout { VK_IMAGE_LAYOUT_UNDEFINED };
  VkSampler sampler{ nullptr };

  class BufferData * bufferdata { nullptr };

  std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
  std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
  std::vector<VkWriteDescriptorSet> write_descriptor_sets;
  std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
  std::vector<VkVertexInputBindingDescription> vertex_input_bindings;
  std::vector<VkVertexInputAttributeDescription> vertex_attributes;
};

struct RecordState {
  VkBuffer buffer{ nullptr };
  class BufferData * bufferdata { nullptr };
  VkFramebuffer framebuffer;

  VulkanIndexBufferDescription index_buffer_description;
  std::vector<VkBuffer> vertex_attribute_buffers;
  std::vector<VkDeviceSize> vertex_attribute_buffer_offsets;
};

struct RenderState {
  VkFramebuffer framebuffer;

  mat4d ModelMatrix{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  };
};
