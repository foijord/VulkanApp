#pragma once

#include <Innovator/core/Vulkan/Wrapper.h>
#include <Innovator/core/State.h>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <gli/texture.hpp>

#include <map>
#include <vector>
#include <string>
#include <memory>

class VulkanBufferObject {
public:
  VulkanBufferObject(const std::shared_ptr<VulkanDevice> & device,
                     size_t size,
                     VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags memory_property_flags)
  {
    this->buffer = std::make_shared<VulkanBuffer>(device, 
                                                  0, 
                                                  size, 
                                                  usage, 
                                                  VK_SHARING_MODE_EXCLUSIVE);
    
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device->device, this->buffer->buffer, &memory_requirements);
    this->memory = std::make_unique<VulkanBufferMemory>(this->buffer, 
                                                        memory_requirements, 
                                                        memory_property_flags);
  }

  ~VulkanBufferObject() {}

  std::shared_ptr<VulkanBuffer> buffer;
  std::unique_ptr<VulkanMemory> memory;
};

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
    this->image = std::make_shared<VulkanImage>(
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

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(this->device->device, this->image->image, &memory_requirements);
    this->memory = std::make_unique<VulkanImageMemory>(
      this->image,
      memory_requirements,
      memory_property_flags);

    this->view = std::make_unique<VulkanImageView>(
      this->image,
      format,
      view_type,
      component_mapping,
      subresource_range);
  }

  ~ImageObject() {}

  void setData(VkCommandBuffer command, const gli::texture & texture)
  {
    VkImageMemoryBarrier memory_barrier {
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

    this->cpu_buffer = std::make_unique<VulkanBufferObject>(this->device, 
                                                            texture.size(), 
                                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    this->cpu_buffer->memory->memcpy(texture.data(), texture.size());

    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < texture.levels(); mip_level++) {
      VkExtent3D image_extent {
        static_cast<uint32_t>(texture.extent(mip_level).x),
        static_cast<uint32_t>(texture.extent(mip_level).y),
        static_cast<uint32_t>(texture.extent(mip_level).z)
      };

      VkOffset3D image_offset = {
        0, 0, 0
      };

      VkImageSubresourceLayers subresource_layers {
        this->subresource_range.aspectMask,     // aspectMask
        mip_level,                              // mipLevel
        this->subresource_range.baseArrayLayer, // baseArrayLayer
        this->subresource_range.layerCount      // layerCount
      };

      VkBufferImageCopy buffer_image_copy {
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

  std::shared_ptr<VulkanImage> image;
  std::unique_ptr<VulkanMemory> memory;
  std::unique_ptr<VulkanImageView> view;

  std::unique_ptr<VulkanBufferObject> cpu_buffer;
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
        texture.type,                   // type 
        1,                              // descriptorCount
      });
      descriptor_set_layout_bindings.push_back({
        texture.binding,                 // binding 
        texture.type,                    // descriptorType 
        1,                               // descriptorCount 
        texture.stage,                   // stageFlags 
        nullptr                          // pImmutableSamplers 
      });
    }

    for (const VulkanBufferDescription & buffer : buffers) {
      descriptor_pool_sizes.push_back({
        buffer.type,                     // type
        1,                               // descriptorCount
      });

      descriptor_set_layout_bindings.push_back({
        buffer.binding,                    // binding
        buffer.type,                       // descriptorType
        1,                                 // descriptorCount
        buffer.stage,                      // stageFlags
        nullptr,                           // pImmutableSamplers
      });
    }

    this->descriptor_set_layout = std::make_unique<VulkanDescriptorSetLayout>(device, descriptor_set_layout_bindings);
    this->descriptor_pool = std::make_unique<VulkanDescriptorPool>(device, descriptor_pool_sizes);
    this->descriptor_set = std::make_unique<VulkanDescriptorSets>(device, this->descriptor_pool, 1, descriptor_set_layout->layout);
    
    std::vector<VkDescriptorSetLayout> setlayouts { descriptor_set_layout->layout };
    this->pipeline_layout = std::make_unique<VulkanPipelineLayout>(device, setlayouts);

    for (const VulkanTextureDescription & texture : textures) {
      VkDescriptorImageInfo image_info {
        texture.sampler,                          // sampler
        texture.view,                             // imageView
        VK_IMAGE_LAYOUT_GENERAL,                  // imageLayout
      };

      VkWriteDescriptorSet write_descriptor_set {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
        nullptr,                                  // pNext
        this->descriptor_set->descriptor_sets[0], // dstSet
        texture.binding,                          // dstBinding
        0,                                        // dstArrayElement
        1,                                        // descriptorCount
        texture.type,                             // descriptorType
        &image_info,                              // pImageInfo
        nullptr,                                  // pBufferInfo
        nullptr,                                  // pTexelBufferView
      };

      this->descriptor_set->update({ write_descriptor_set });
    }

    for (const VulkanBufferDescription & buffer : buffers) {
      VkDescriptorBufferInfo buffer_info {
        buffer.buffer,                            // buffer
        0,                                        // offset
        buffer.size,                              // range
      };

      VkWriteDescriptorSet write_descriptor_set {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,   // sType
        nullptr,                                  // pNext
        this->descriptor_set->descriptor_sets[0], // dstSet
        buffer.binding,                           // dstBinding
        0,                                        // dstArrayElement
        1,                                        // descriptorCount
        buffer.type,                              // descriptorType
        nullptr,                                  // pImageInfo
        &buffer_info,                             // pBufferInfo
        nullptr,                                  // pTexelBufferView
      };

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
                         const VkPipelineRasterizationStateCreateInfo & rasterizationstate,
                         const std::shared_ptr<VulkanRenderpass> & renderpass,
                         const std::unique_ptr<VulkanPipelineCache> & pipelinecache,
                         const VulkanDrawDescription & drawdescription)
    : device(device)
  {
    this->descriptor_set = std::make_unique<DescriptorSetObject>(device, textures, buffers);

    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_infos;
    for (const VulkanShaderModuleDescription & shader : shaders) {
      shader_stage_infos.push_back({
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType 
        nullptr,                                             // pNext
        0,                                                   // flags (reserved for future use)
        shader.stage,                                        // stage
        shader.module,                                       // module 
        "main",                                              // pName 
        nullptr,                                             // pSpecializationInfo
      });
    }

    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

    for (const VulkanVertexAttributeDescription & attribute : attributes) {
      attribute_descriptions.push_back({
        attribute.location, // location
        attribute.binding,  // binding
        attribute.format,   // format
        attribute.offset,   // offset
      });
      binding_descriptions.push_back({
        attribute.binding,    // binding
        attribute.stride,     // stride
        attribute.input_rate, //inputRate
      });
    }

    std::vector<VkDynamicState> dynamic_states { 
      VK_DYNAMIC_STATE_VIEWPORT, 
      VK_DYNAMIC_STATE_SCISSOR 
    };

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

class ComputePipelineObject {
public:
  ComputePipelineObject(const std::shared_ptr<VulkanDevice> & device, 
                        const VulkanShaderModuleDescription & shader,
                        const std::vector<VulkanBufferDescription> & buffers,
                        const std::vector<VulkanTextureDescription> textures,
                        const std::unique_ptr<VulkanPipelineCache> & pipelinecache)
  {
    this->descriptor_set = std::make_unique<DescriptorSetObject>(device, textures, buffers);

    VkPipelineShaderStageCreateInfo shader_stage_info {
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType 
      nullptr,                                             // pNext 
      0,                                                   // flags (reserved for future use)
      shader.stage,                                        // stage 
      shader.module,                                       // module 
      "main",                                              // pName 
      nullptr,                                             // pSpecializationInfo
    };

    this->pipeline = std::make_unique<VulkanComputePipeline>(
      device,
      pipelinecache->cache,
      0,
      shader_stage_info,
      this->descriptor_set->pipeline_layout->layout);
  }

  ~ComputePipelineObject() {}

  void bind(VkCommandBuffer command)
  {
    this->descriptor_set->bind(command, VK_PIPELINE_BIND_POINT_COMPUTE);
    this->pipeline->bind(command);
  }

  std::unique_ptr<DescriptorSetObject> descriptor_set;
  std::unique_ptr<VulkanComputePipeline> pipeline;
};