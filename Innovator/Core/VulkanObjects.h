#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Vulkan/Wrapper.h>
#include <Innovator/Core/State.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <vector>
#include <memory>

class BufferObject {
public:
  NO_COPY_OR_ASSIGNMENT(BufferObject);
  BufferObject() = delete;
  ~BufferObject() = default;
  
  BufferObject(std::shared_ptr<VulkanBuffer> buffer,
               VkMemoryPropertyFlags memory_property_flags)
  {
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(buffer->device->device, buffer->buffer, &memory_requirements);

    uint32_t memory_type_index = buffer->device->physical_device.getMemoryTypeIndex(memory_requirements,
                                                                                    memory_property_flags);

    this->memory = std::make_unique<VulkanMemory>(buffer->device,
                                                  memory_requirements.size,
                                                  memory_type_index);

    const VkDeviceSize offset = 0;

    THROW_ON_ERROR(vkBindBufferMemory(buffer->device->device,
                                      buffer->buffer, 
                                      this->memory->memory, 
                                      offset));
  }

  std::shared_ptr<VulkanMemory> memory;
};

class ImageObject {
public:
  NO_COPY_OR_ASSIGNMENT(ImageObject);
  ImageObject() = delete;
  ~ImageObject() = default;

  ImageObject(std::shared_ptr<VulkanImage> image,
              VkMemoryPropertyFlags memory_property_flags)
  {
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(
      image->device->device,
      image->image,
      &memory_requirements);

    uint32_t memory_type_index = image->device->physical_device.getMemoryTypeIndex(
      memory_requirements,
      memory_property_flags);

    this->memory = std::make_unique<VulkanMemory>(
      image->device,
      memory_requirements.size,
      memory_type_index);

    const VkDeviceSize offset = 0;

    THROW_ON_ERROR(vkBindImageMemory(
      image->device->device,
      image->image,
      this->memory->memory,
      offset));
  }

  std::unique_ptr<VulkanMemory> memory;
};

class DescriptorSetObject {
public:
  NO_COPY_OR_ASSIGNMENT(DescriptorSetObject);
  DescriptorSetObject() = delete;
  ~DescriptorSetObject() = default;

  DescriptorSetObject(const std::shared_ptr<VulkanDevice> & device, 
                      const std::vector<VulkanTextureDescription> & textures,
                      const std::vector<VulkanBufferDescription> & buffers)
  {
    std::vector<VkDescriptorPoolSize> descriptor_pool_sizes;
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;

    for (const VulkanTextureDescription & texture : textures) {
      descriptor_pool_sizes.push_back({
        texture.layout.type,                    // type 
        1,                                      // descriptorCount
      });
      descriptor_set_layout_bindings.push_back({
        texture.layout.binding,                 // binding 
        texture.layout.type,                    // descriptorType 
        1,                                      // descriptorCount 
        texture.layout.stage,                   // stageFlags 
        nullptr                                 // pImmutableSamplers 
      });
    }

    for (const VulkanBufferDescription & buffer : buffers) {
      descriptor_pool_sizes.push_back({
        buffer.layout.type,              // type
        1,                               // descriptorCount
      });

      descriptor_set_layout_bindings.push_back({
        buffer.layout.binding,             // binding
        buffer.layout.type,                // descriptorType
        1,                                 // descriptorCount
        buffer.layout.stage,               // stageFlags
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
        texture.layout.binding,                   // dstBinding
        0,                                        // dstArrayElement
        1,                                        // descriptorCount
        texture.layout.type,                      // descriptorType
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
        buffer.layout.binding,                    // dstBinding
        0,                                        // dstArrayElement
        1,                                        // descriptorCount
        buffer.layout.type,                       // descriptorType
        nullptr,                                  // pImageInfo
        &buffer_info,                             // pBufferInfo
        nullptr,                                  // pTexelBufferView
      };

      this->descriptor_set->update({ write_descriptor_set });
    }
  }


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
  NO_COPY_OR_ASSIGNMENT(GraphicsPipelineObject);
  GraphicsPipelineObject() = delete;
  ~GraphicsPipelineObject() = default;

  GraphicsPipelineObject(std::shared_ptr<VulkanDevice> device,
                         const std::vector<VulkanVertexAttributeDescription> & attributes,
                         const std::vector<VulkanShaderModuleDescription> & shaders,
                         const std::vector<VulkanBufferDescription> & buffers,
                         const std::vector<VulkanTextureDescription> & textures,
                         const VkPipelineRasterizationStateCreateInfo & rasterizationstate,
                         const std::shared_ptr<VulkanRenderpass> & renderpass,
                         const std::unique_ptr<VulkanPipelineCache> & pipelinecache,
                         const VulkanDrawDescription & drawdescription)
    : device(std::move(device))
  {
    this->descriptor_set = std::make_unique<DescriptorSetObject>(this->device, textures, buffers);

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
        attribute.inputrate, //inputRate
      });
    }

    std::vector<VkDynamicState> dynamic_states { 
      VK_DYNAMIC_STATE_VIEWPORT, 
      VK_DYNAMIC_STATE_SCISSOR 
    };

    this->pipeline = std::make_unique<VulkanGraphicsPipeline>(
      this->device,
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
  NO_COPY_OR_ASSIGNMENT(ComputePipelineObject);
  ComputePipelineObject() = delete;
  ~ComputePipelineObject() = default;

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


  void bind(VkCommandBuffer command)
  {
    this->descriptor_set->bind(command, VK_PIPELINE_BIND_POINT_COMPUTE);
    this->pipeline->bind(command);
  }

  std::unique_ptr<DescriptorSetObject> descriptor_set;
  std::unique_ptr<VulkanComputePipeline> pipeline;
};