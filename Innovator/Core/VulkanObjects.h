#pragma once

#include <Innovator/core/Vulkan/Wrapper.h>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <gli/texture.hpp>

#include <map>
#include <vector>
#include <string>
#include <memory>

//*******************************************************************************************************************************
class VulkanDebugCallback {
public:
  VulkanDebugCallback(const std::shared_ptr<VulkanInstance> & vulkan, 
                      VkDebugReportFlagsEXT flags,
                      PFN_vkDebugReportCallbackEXT callback,
                      void * pUserData = nullptr)
    : vulkan(vulkan)
  {
    VkDebugReportCallbackCreateInfoEXT create_info;
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    create_info.pNext = nullptr;
    create_info.flags = flags;
    create_info.pfnCallback = callback;

    THROW_ERROR(this->vulkan->vkCreateDebugReportCallback(this->vulkan->instance, &create_info, nullptr, &this->callback));
  }

  ~VulkanDebugCallback()
  {
    this->vulkan->vkDestroyDebugReportCallback(this->vulkan->instance, this->callback, nullptr);
  }

  VkDebugReportCallbackEXT callback;
  std::shared_ptr<VulkanInstance> vulkan;
};

//*******************************************************************************************************************************
class CommandBufferRecorder {
public:
  CommandBufferRecorder(VulkanCommandBuffers * buffer, uint32_t index = 0)
    : buffer(buffer), index(index)
  {
    this->buffer->begin(this->index);
  }

  ~CommandBufferRecorder()
  {
    try {
      this->buffer->end(this->index);
    }
    catch (std::exception & e) {
      MessageBox(nullptr, e.what(), "Alert", MB_OK);
    }
  }

  VkCommandBuffer & command()
  {
    return this->buffer->buffers[this->index];
  }

  uint32_t index;
  class VulkanCommandBuffers * buffer;
};

//*******************************************************************************************************************************
class ImageObject {
public:
  ImageObject(const std::shared_ptr<VulkanDevice> & device,
              VkFormat format,
              VkExtent3D extent,
              VkImageLayout layout,
              VkImageTiling tiling,
              VkImageType image_type, 
              VkImageUsageFlags usage,
              VkMemoryPropertyFlags memory_property_flags,
              VkImageSubresourceRange subresource_range,
              VkComponentMapping component_mapping)
    : extent(extent), layout(layout), subresource_range(subresource_range)
  {
    this->image = std::make_unique<VulkanImage>(
      device,
      0,
      image_type,
      format,
      extent,
      subresource_range.levelCount,
      subresource_range.layerCount,
      VK_SAMPLE_COUNT_1_BIT,
      tiling,
      usage,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>(),
      layout);

    this->memory = std::make_unique<VulkanMemory>(
      device,
      this->image,
      memory_property_flags);

    this->view = std::make_unique<VulkanImageView>(
      device,
      this->image->image,
      format,
      (VkImageViewType)image_type,
      component_mapping,
      this->subresource_range);
  }


  ~ImageObject() {}

  void copyBufferToImage(VkCommandBuffer command, VkBuffer buffer, uint32_t mipLevel, VkExtent3D extent, VkDeviceSize bufferOffset)
  {
    VkBufferImageCopy buffer_image_copy;
    ::memset(&buffer_image_copy, 0, sizeof(VkBufferImageCopy));
    buffer_image_copy.imageSubresource.mipLevel = mipLevel;
    buffer_image_copy.imageSubresource.aspectMask = this->subresource_range.aspectMask;
    buffer_image_copy.imageSubresource.baseArrayLayer = this->subresource_range.baseArrayLayer;
    buffer_image_copy.imageSubresource.layerCount = this->subresource_range.layerCount;
    buffer_image_copy.imageExtent = extent;
    buffer_image_copy.bufferOffset = bufferOffset;

    vkCmdCopyBufferToImage(command, buffer, this->image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
  }

  void copyImageToImage(VkCommandBuffer command, VkImage image)
  {
    VkImageCopy image_copy;
    ::memset(&image_copy, 0, sizeof(VkImageCopy));

    image_copy.srcSubresource.aspectMask = this->subresource_range.aspectMask;
    image_copy.srcSubresource.mipLevel = this->subresource_range.baseMipLevel;
    image_copy.srcSubresource.baseArrayLayer = this->subresource_range.baseArrayLayer;
    image_copy.srcSubresource.layerCount = this->subresource_range.layerCount;
    image_copy.dstSubresource = image_copy.srcSubresource;
    image_copy.extent = this->extent;

    vkCmdCopyImage(command, this->image->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_copy);
  }

  VkImageMemoryBarrier setLayout(VkImageLayout new_layout)
  {
    std::map<VkImageLayout, VkAccessFlags> dst_access_mask{
      { VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
      { VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT },
      { VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT },
    };

    VkImageMemoryBarrier memory_barrier;
    ::memset(&memory_barrier, 0, sizeof(VkImageMemoryBarrier));
    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.image = this->image->image;
    memory_barrier.subresourceRange = this->subresource_range;
    memory_barrier.dstAccessMask = dst_access_mask[new_layout];
    memory_barrier.oldLayout = this->layout;
    this->layout = new_layout;
    memory_barrier.newLayout = this->layout;

    return memory_barrier;
  }

  std::unique_ptr<VulkanImage> image;
  std::unique_ptr<VulkanMemory> memory;
  std::unique_ptr<VulkanImageView> view;
  VkExtent3D extent;
  VkImageLayout layout;
  VkImageSubresourceRange subresource_range;
};


//*******************************************************************************************************************************
class TextureObject {
public:
  TextureObject(const std::shared_ptr<VulkanDevice> & device,
                VkComponentMapping components,
                VkImageSubresourceRange subresource_range,
                VkImageType image_type,
                VkFormat format,
                VkExtent3D extent)
    : device(device)
  {
    this->sampler = std::make_unique<VulkanSampler>(
      this->device,
      VK_FILTER_LINEAR,
      VK_FILTER_LINEAR,
      VK_SAMPLER_MIPMAP_MODE_LINEAR,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      0.f,
      VK_FALSE,
      1.f,
      VK_FALSE,
      VK_COMPARE_OP_NEVER,
      0.f,
      0.f,
      VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
      VK_FALSE);

    this->image_object = std::make_unique<ImageObject>(
      this->device,
      format,
      extent,
      VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_TILING_OPTIMAL,
      image_type,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      subresource_range,
      components);
  }

  ~TextureObject() {}

  void setData(VkCommandBuffer command, const gli::texture & texture)
  {
    VkImageMemoryBarrier memory_barrier = this->image_object->setLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &memory_barrier);

    this->staging_buffer = std::make_unique<VulkanBuffer>(this->device, 0, texture.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, std::vector<uint32_t>());
    this->staging_memory = std::make_unique<VulkanMemory>(this->device, this->staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    this->staging_memory->memcpy(texture.data(), texture.size());

    VkDeviceSize buffer_offset = 0;
    for (uint32_t i = 0; i < texture.levels(); i++) {
      gli::extent3d extent3 = texture.extent(i);
      VkExtent3D extent = {
        static_cast<uint32_t>(extent3.x),
        static_cast<uint32_t>(extent3.y),
        static_cast<uint32_t>(extent3.z)
      };
      this->image_object->copyBufferToImage(command, this->staging_buffer->buffer, i, extent, buffer_offset);
      buffer_offset += texture.size(i);
    }
  }

  std::shared_ptr<VulkanDevice> device;

  std::unique_ptr<VulkanBuffer> staging_buffer;
  std::unique_ptr<VulkanMemory> staging_memory;

  std::unique_ptr<VulkanSampler> sampler;
  std::unique_ptr<ImageObject> image_object;
};


//*******************************************************************************************************************************
template <typename T>
class VulkanHostVisibleBufferObject;

template <typename T>
class VulkanDeviceLocalBufferObject;

template <typename T>
class VulkanBufferObject {
public:
  VulkanBufferObject(const std::shared_ptr<VulkanDevice> & device,
                     size_t size,
                     VkBufferUsageFlagBits usage,
                     VkMemoryPropertyFlags property_flags)
      : buffer(std::make_unique<VulkanBuffer>(device, 0, size, usage, VK_SHARING_MODE_EXCLUSIVE, std::vector<uint32_t>())),
        memory(std::make_unique<VulkanMemory>(device, this->buffer, property_flags))
  {}

  ~VulkanBufferObject() {}

  std::unique_ptr<VulkanBuffer> buffer;
  std::unique_ptr<VulkanMemory> memory;
};

template <typename T>
class VulkanHostVisibleBufferObject : public VulkanBufferObject<T> {
public:
  VulkanHostVisibleBufferObject(const std::shared_ptr<VulkanDevice> & device, size_t size, VkBufferUsageFlagBits usage)
    : VulkanBufferObject(device, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {}
  ~VulkanHostVisibleBufferObject() {}

  void setValues(const std::vector<T> & values)
  {
    this->memory->memcpy(values.data(), sizeof(T) * values.size());
  }
};

template <typename T>
class VulkanDeviceLocalBufferObject : public VulkanBufferObject<T> {
public:
  VulkanDeviceLocalBufferObject(const std::shared_ptr<VulkanDevice> & device, size_t size, VkBufferUsageFlagBits usage)
    : VulkanBufferObject(device, size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}
  VulkanDeviceLocalBufferObject() {}

  void setValues(VkCommandBuffer command, const std::unique_ptr<VulkanHostVisibleBufferObject<T>> & src_buffer, VkDeviceSize size)
  {
    VkBufferCopy region;
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size = size;

    vkCmdCopyBuffer(command, src_buffer->buffer->buffer, this->buffer->buffer, 1, &region);
  }
};

//*******************************************************************************************************************************
class VulkanLayoutBinding {
public:
  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
};

//*******************************************************************************************************************************
class VulkanVertexAttributeDescription {
public:
  uint32_t location;
  uint32_t binding;
  VkFormat format;
  uint32_t offset;
  uint32_t stride;
  VkVertexInputRate input_rate;
  VkBuffer buffer;
};

//*******************************************************************************************************************************
class VulkanIndexBufferDescription {
public:
  uint32_t count;
  VkBuffer buffer;
  VkIndexType type;
};

//*******************************************************************************************************************************
class VulkanBufferDescription {
public:
  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
  VkBuffer buffer;
  VkDeviceSize size;
  VkDeviceSize offset;
};

//*******************************************************************************************************************************
class VulkanTextureDescription {
public:
  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
  VkImageView view;
  VkSampler sampler;
};

//*******************************************************************************************************************************
class VulkanShaderModuleDescription {
public:
  VkShaderStageFlagBits stage;
  VkShaderModule module;
};

//*******************************************************************************************************************************
class VulkanDrawDescription {
public:
  uint32_t count;
  VkPrimitiveTopology topology;
};

//*******************************************************************************************************************************
class VulkanComputeDescription {
public:
  uint32_t group_count_x;
  uint32_t group_count_y;
  uint32_t group_count_z;
};

class DescriptorSetObject {
public:
  DescriptorSetObject(const std::shared_ptr<VulkanDevice> & device, 
                      const std::vector<VulkanTextureDescription> & textures,
                      const std::vector<VulkanBufferDescription> & buffers)
  {
    std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;

    for (const VulkanTextureDescription & texture : textures) {
      VkDescriptorPoolSize descriptor_pool_size;
      ::memset(&descriptor_pool_size, 0, sizeof(VkDescriptorPoolSize));

      descriptor_pool_size.type = texture.type;
      descriptor_pool_size.descriptorCount = 1;
      descriptor_pool_sizes.push_back(descriptor_pool_size);

      VkDescriptorSetLayoutBinding layout_binding;
      ::memset(&layout_binding, 0, sizeof(VkDescriptorSetLayoutBinding));

      layout_binding.descriptorType = texture.type;
      layout_binding.binding = texture.binding;
      layout_binding.descriptorCount = 1;
      layout_binding.stageFlags = texture.stage;
      descriptor_set_layout_bindings.push_back(layout_binding);
    }

    for (const VulkanBufferDescription & buffer : buffers) {
      VkDescriptorPoolSize descriptor_pool_size;
      ::memset(&descriptor_pool_size, 0, sizeof(VkDescriptorPoolSize));

      descriptor_pool_size.type = buffer.type;
      descriptor_pool_size.descriptorCount = 1;
      descriptor_pool_sizes.push_back(descriptor_pool_size);

      VkDescriptorSetLayoutBinding layout_binding;
      ::memset(&layout_binding, 0, sizeof(VkDescriptorSetLayoutBinding));

      layout_binding.descriptorType = buffer.type;
      layout_binding.binding = buffer.binding;
      layout_binding.descriptorCount = 1;
      layout_binding.stageFlags = buffer.stage;
      descriptor_set_layout_bindings.push_back(layout_binding);
    }

    this->descriptor_set_layout = std::make_unique<VulkanDescriptorSetLayout>(device, descriptor_set_layout_bindings);
    this->descriptor_pool = std::make_unique<VulkanDescriptorPool>(device, descriptor_pool_sizes);
    this->descriptor_set = std::make_unique<VulkanDescriptorSets>(device, this->descriptor_pool, 1, descriptor_set_layout->layout);
    std::vector<VkDescriptorSetLayout> setlayouts;
    setlayouts.push_back(descriptor_set_layout->layout);
    this->pipeline_layout = std::make_unique<VulkanPipelineLayout>(device, setlayouts);

    for (const VulkanTextureDescription & texture : textures) {
      VkDescriptorImageInfo image_info;
      ::memset(&image_info, 0, sizeof(VkDescriptorImageInfo));

      image_info.imageView = texture.view;
      image_info.sampler = texture.sampler;
      image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

      VkWriteDescriptorSet write_descriptor_set;
      ::memset(&write_descriptor_set, 0, sizeof(VkWriteDescriptorSet));
      write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

      write_descriptor_set.descriptorType = texture.type;
      write_descriptor_set.descriptorCount = 1;
      write_descriptor_set.pImageInfo = &image_info;
      write_descriptor_set.dstBinding = texture.binding;
      write_descriptor_set.dstSet = this->descriptor_set->descriptor_sets[0];

      this->descriptor_set->update({ write_descriptor_set });
    }

    for (const VulkanBufferDescription & buffer : buffers) {
      VkDescriptorBufferInfo buffer_info;
      ::memset(&buffer_info, 0, sizeof(VkDescriptorBufferInfo));
      buffer_info.buffer = buffer.buffer;
      buffer_info.offset = 0;
      buffer_info.range = buffer.size;

      VkWriteDescriptorSet write_descriptor_set;
      ::memset(&write_descriptor_set, 0, sizeof(VkWriteDescriptorSet));
      write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

      write_descriptor_set.descriptorType = buffer.type;
      write_descriptor_set.descriptorCount = 1;
      write_descriptor_set.pBufferInfo = &buffer_info;
      write_descriptor_set.dstBinding = buffer.binding;
      write_descriptor_set.dstSet = this->descriptor_set->descriptor_sets[0];

      this->descriptor_set->update({ write_descriptor_set });
    }
  }

  ~DescriptorSetObject() {}

  void bind(VkCommandBuffer command, VkPipelineBindPoint bind_point)
  {
    this->descriptor_set->bind(command, bind_point, this->pipeline_layout->layout);
  }

  std::shared_ptr<VulkanDescriptorPool> descriptor_pool;
  std::shared_ptr<VulkanDescriptorSetLayout> descriptor_set_layout;
  std::shared_ptr<VulkanDescriptorSets> descriptor_set;
  std::shared_ptr<VulkanPipelineLayout> pipeline_layout;
};

class GraphicsPipelineObject {
public:
  GraphicsPipelineObject(const std::shared_ptr<VulkanDevice> & device,
                         const std::vector<VulkanVertexAttributeDescription> & attributes,
                         const std::vector<VulkanShaderModuleDescription> & shaders,
                         VkPipelineRasterizationStateCreateInfo rasterization_state,
                         VkPipelineLayout pipeline_layout,
                         VkRenderPass render_pass,
                         VkPipelineCache pipeline_cache,
                         VkPrimitiveTopology topology)
    : device(device)
  {
    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
    for (const VulkanShaderModuleDescription & shader : shaders) {
      VkPipelineShaderStageCreateInfo shader_stage_info;
      ::memset(&shader_stage_info, 0, sizeof(VkPipelineShaderStageCreateInfo));
      shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shader_stage_info.stage = shader.stage;
      shader_stage_info.module = shader.module;
      shader_stage_info.pName = "main";

      shader_stage_infos.push_back(shader_stage_info);
    }
    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

    for (const VulkanVertexAttributeDescription & attribute : attributes) {
      VkVertexInputAttributeDescription attribute_description;
      attribute_description.binding = attribute.binding;
      attribute_description.location = attribute.location;
      attribute_description.format = attribute.format;
      attribute_description.offset = attribute.offset;

      VkVertexInputBindingDescription binding_description;
      binding_description.binding = attribute.binding;
      binding_description.stride = attribute.stride;
      binding_description.inputRate = attribute.input_rate;

      binding_descriptions.push_back(binding_description);
      attribute_descriptions.push_back(attribute_description);
    }
    std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    this->pipeline = std::make_unique<VulkanGraphicsPipeline>(
      device,
      render_pass,
      pipeline_cache,
      pipeline_layout,
      topology,
      rasterization_state,
      dynamic_states,
      shader_stage_infos,
      binding_descriptions,
      attribute_descriptions);
  }

  ~GraphicsPipelineObject() {}

  void bind(VkCommandBuffer command)
  {
    this->pipeline->bind(command);
  }

  std::shared_ptr<VulkanDevice> device;
  std::unique_ptr<VulkanGraphicsPipeline> pipeline;
};
