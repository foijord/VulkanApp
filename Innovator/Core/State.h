#pragma once

#include <Innovator/Core/Vulkan/Wrapper.h>

#include <glm/mat4x4.hpp>
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
  VkVertexInputRate input_rate;
  VkBuffer buffer;
};

struct VulkanIndexBufferDescription {
  VkIndexType type;
  VkBuffer buffer;
  uint32_t count;
};

struct VulkanBufferDescription {
  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
  VkBuffer buffer;
  VkDeviceSize size;
  VkDeviceSize offset;
};

struct VulkanTextureDescription {
  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
  VkImageView view;
  VkSampler sampler;
};

struct VulkanShaderModuleDescription {
  VkShaderStageFlagBits stage;
  VkShaderModule module;
};

struct VulkanDrawDescription {
  uint32_t count;
  VkPrimitiveTopology topology;
};

struct VulkanComputeDescription {
  uint32_t group_count_x;
  uint32_t group_count_y;
  uint32_t group_count_z;
};

class State {
public:
  State() 
    : ViewMatrix(glm::mat4(1)),
      ProjMatrix(glm::mat4(1)),
      ModelMatrix(glm::mat4(1)),
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
  
  glm::mat4 ViewMatrix;
  glm::mat4 ProjMatrix;
  glm::mat4 ModelMatrix;

  VulkanLayoutBinding layout_binding;
  VulkanDrawDescription drawdescription;
  VulkanTextureDescription texture_description;
  VulkanComputeDescription compute_description;
  VulkanVertexAttributeDescription attribute_description;
  VkPipelineRasterizationStateCreateInfo rasterizationstate;

  std::vector<VulkanBufferDescription> buffers;
  std::vector<VulkanTextureDescription> textures;
  std::vector<VulkanIndexBufferDescription> indices;
  std::vector<VulkanShaderModuleDescription> shaders;
  std::vector<VulkanVertexAttributeDescription> attributes;
};