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
  VkBuffer buffer;
  VkDeviceSize size;
  VkDeviceSize offset;
};

struct VulkanTextureDescription {
  VulkanLayoutBinding layout;
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

struct VulkanBufferDataDescription {
  VkDeviceSize size{ 0 };
  VkDeviceSize offset{ 0 };
  size_t count{ 0 };
  VkDeviceSize elem_size{ 0 };
  VkBufferUsageFlags usage_flags{ 0 };
  VkMemoryPropertyFlags memory_property_flags{ 0 };
  void * data{ nullptr };
  VkBuffer buffer{ nullptr };
};

struct VulkanImageDataDescription {
  VkImageUsageFlags usage_flags{ 0 };
  VkMemoryPropertyFlags memory_property_flags{ 0 };
  gli::texture * texture{ nullptr };
  VkImage image{ nullptr };
};

class RenderState {
public:
  RenderState() :
    ViewMatrix(glm::mat4(1)),
    ProjMatrix(glm::mat4(1)),
    ModelMatrix(glm::mat4(1))
  {}

  VkViewport viewport;
  glm::mat4 ViewMatrix;
  glm::mat4 ProjMatrix;
  glm::mat4 ModelMatrix;
};

class State {
public:
  State() : 
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
  
  VulkanLayoutBinding layout_binding;
  VulkanDrawDescription drawdescription;
  VulkanImageDataDescription imagedata;
  VulkanBufferDataDescription bufferdata;
  VulkanTextureDescription texture_description;
  VulkanComputeDescription compute_description;
  VulkanVertexAttributeDescription attribute_description;
  VkPipelineRasterizationStateCreateInfo rasterizationstate;

  std::vector<VulkanTextureDescription> textures;
  std::vector<VulkanIndexBufferDescription> indices;
  std::vector<VulkanShaderModuleDescription> shaders;
  std::vector<VulkanBufferDescription> buffer_descriptions;
  std::vector<VulkanImageDataDescription> image_descriptions;
  std::vector<VulkanBufferDataDescription> bufferdata_descriptions;
  std::vector<VulkanVertexAttributeDescription> attribute_descriptions;
};
