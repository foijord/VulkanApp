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
class VulkanBufferObject {
public:
  VulkanBufferObject(const std::shared_ptr<VulkanDevice> & device,
                     size_t size,
                     VkBufferUsageFlagBits usage,
                     VkMemoryPropertyFlags property_flags)
    : buffer(std::make_unique<VulkanBuffer>(device, 0, size, usage, VK_SHARING_MODE_EXCLUSIVE)),
      memory(std::make_unique<VulkanMemory>(device, this->buffer, property_flags))
  {}

  ~VulkanBufferObject() {}

  void setValues(const void * data, size_t size) 
  {
    this->memory->memcpy(data, size);
  }

  std::unique_ptr<VulkanBuffer> buffer;
  std::unique_ptr<VulkanMemory> memory;
};

template <typename T>
class VulkanHostVisibleBufferObject : public VulkanBufferObject {
public:
  VulkanHostVisibleBufferObject(const std::shared_ptr<VulkanDevice> & device, size_t size, VkBufferUsageFlagBits usage)
    : VulkanBufferObject(device, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {}
  ~VulkanHostVisibleBufferObject() {}

  void setValues(const std::vector<T> & values)
  {
    VulkanBufferObject::setValues(values.data(), sizeof(T) * values.size());
  }
};

template <typename T>
class VulkanDeviceLocalBufferObject : public VulkanBufferObject {
public:
  VulkanDeviceLocalBufferObject(const std::shared_ptr<VulkanDevice> & device, size_t size, VkBufferUsageFlagBits usage)
    : VulkanBufferObject(device, size, usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {}
  VulkanDeviceLocalBufferObject() {}

  void copy(VkCommandBuffer command, const std::unique_ptr<VulkanBuffer> & src_buffer, VkDeviceSize size)
  {
    VkBufferCopy region = {
      0,    // srcOffset
      0,    // dstOffset 
      size, // size 
    };
    vkCmdCopyBuffer(command, src_buffer->buffer, this->buffer->buffer, 1, &region);
  }
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
              VkImageViewType view_type,
              VkSampleCountFlagBits samples,
              VkImageUsageFlags usage,
              VkMemoryPropertyFlags memory_property_flags,
              VkImageSubresourceRange subresource_range,
              VkComponentMapping component_mapping)
    : device(device),
      extent(extent), 
      layout(layout), 
      subresource_range(subresource_range)
  {
    this->image = std::make_unique<VulkanImage>(
      device,
      0,
      image_type,
      format,
      extent,
      subresource_range.levelCount,
      subresource_range.layerCount,
      samples,
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
      view_type,
      component_mapping,
      this->subresource_range);
  }

  ~ImageObject() {}

  void setData(VkCommandBuffer command, const gli::texture & texture)
  {
    VkImageMemoryBarrier memory_barrier = {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
      nullptr,                                                       // pNext
      0,                                                             // srcAccessMask
      VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
      VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // newLayout
      this->device->default_queue_index,                             // srcQueueFamilyIndex
      this->device->default_queue_index,                             // dstQueueFamilyIndex
      this->image->image,                                            // image
      this->subresource_range,                                       // subresourceRange
    };

    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &memory_barrier);

    this->cpu_buffer = std::make_unique<VulkanBufferObject>(this->device, texture.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    this->cpu_buffer->setValues(texture.data(), texture.size());

    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < texture.levels(); mip_level++) {
      VkExtent3D image_extent = {
        static_cast<uint32_t>(texture.extent(mip_level).x),
        static_cast<uint32_t>(texture.extent(mip_level).y),
        static_cast<uint32_t>(texture.extent(mip_level).z)
      };
      VkOffset3D image_offset = {
        0, 0, 0
      };

      VkImageSubresourceLayers subresource_layers = {
        this->subresource_range.aspectMask,     // aspectMask
        mip_level,                              // mipLevel
        this->subresource_range.baseArrayLayer, // baseArrayLayer
        this->subresource_range.layerCount      // layerCount
      };

      VkBufferImageCopy buffer_image_copy = {
        buffer_offset,                             // bufferOffset 
        0,                                         // bufferRowLength
        0,                                         // bufferImageHeight
        subresource_layers,                        // imageSubresource
        image_offset,                              // imageOffset
        image_extent                               // imageExtent
      };

      vkCmdCopyBufferToImage(command, this->cpu_buffer->buffer->buffer, this->image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_image_copy);
      buffer_offset += texture.size(mip_level);
    }
  }

  VkExtent3D extent;
  VkImageLayout layout;
  VkImageSubresourceRange subresource_range;

  std::shared_ptr<VulkanDevice> device;

  std::unique_ptr<VulkanImage> image;
  std::unique_ptr<VulkanMemory> memory;
  std::unique_ptr<VulkanImageView> view;

  std::unique_ptr<VulkanBufferObject> cpu_buffer;
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
      descriptor_pool_sizes.push_back({
        texture.type,         // type 
        1,                    // descriptorCount
      });
      descriptor_set_layout_bindings.push_back({
        texture.binding,               // binding 
        texture.type,                  // descriptorType 
        1,                             // descriptorCount 
        texture.stage,                 // stageFlags 
        nullptr                        // pImmutableSamplers 
      });
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
                         const std::vector<VulkanBufferDescription> & buffers,
                         const std::vector<VulkanTextureDescription> textures,
                         VkPipelineRasterizationStateCreateInfo rasterizationstate,
                         const std::shared_ptr<VulkanRenderpass> & renderpass,
                         const std::unique_ptr<VulkanPipelineCache> & pipelinecache,
                         const VulkanDrawDescription & drawdescription)
    : device(device)
  {
    this->descriptor_set = std::make_unique<DescriptorSetObject>(device, textures, buffers);

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
      renderpass->renderpass,
      pipelinecache->cache,
      this->descriptor_set->pipeline_layout->layout,
      drawdescription.topology,
      rasterizationstate,
      dynamic_states,
      shader_stage_infos,
      binding_descriptions,
      attribute_descriptions);
  }

  ~GraphicsPipelineObject() {}

  void bind(VkCommandBuffer command)
  {
    this->descriptor_set->bind(command, VK_PIPELINE_BIND_POINT_GRAPHICS);
    this->pipeline->bind(command);
  }

  std::shared_ptr<VulkanDevice> device;
  std::unique_ptr<VulkanGraphicsPipeline> pipeline;
  std::unique_ptr<DescriptorSetObject> descriptor_set;
};
