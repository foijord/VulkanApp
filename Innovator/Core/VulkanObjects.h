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
  NO_COPY_OR_ASSIGNMENT(BufferObject)
  BufferObject() = delete;
  ~BufferObject() = default;
  
  BufferObject(VkDeviceSize size,
               VkBufferUsageFlags usage,
               VkSharingMode sharingMode,
               VkMemoryPropertyFlags memory_property_flags,
               VkBufferCreateFlags flags = 0) :
    flags(flags),
    size(size),
    usage(usage),
    sharingMode(sharingMode),
    memory_property_flags(memory_property_flags)    
  {}

  void bind(std::shared_ptr<VulkanBuffer> buffer,
            std::shared_ptr<VulkanMemory> memory, 
            VkDeviceSize offset)
  {
    this->buffer = std::move(buffer);
    this->memory = std::move(memory);
    this->offset = offset;

    THROW_ON_ERROR(vkBindBufferMemory(this->buffer->device->device,
                                      this->buffer->buffer,
                                      this->memory->memory,
                                      this->offset));
  }

  void memcpy(const void * data) const
  {
    this->memory->memcpy(data, this->size, this->offset);
  }

  std::shared_ptr<VulkanBuffer> buffer;

  VkBufferCreateFlags flags;
  VkDeviceSize size;
  VkDeviceSize offset{ 0 };
  VkDeviceSize range{ VK_WHOLE_SIZE };
  VkBufferUsageFlags usage;
  VkSharingMode sharingMode;

  VkMemoryPropertyFlags memory_property_flags;
  std::shared_ptr<VulkanMemory> memory;
};

class ImageObject {
public:
  NO_COPY_OR_ASSIGNMENT(ImageObject)
  ImageObject() = delete;
  ~ImageObject() = default;

  ImageObject(std::shared_ptr<VulkanImage> image,
              VkMemoryPropertyFlags memory_property_flags) :
    image(std::move(image)),
    memory_property_flags(memory_property_flags)
  {}

  void bind()
  {
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(
      this->image->device->device,
      this->image->image,
      &memory_requirements);

    auto memory_type_index = this->image->device->physical_device.getMemoryTypeIndex(
      memory_requirements.memoryTypeBits,
      memory_property_flags);

    const auto memory = std::make_shared<VulkanMemory>(
      this->image->device,
      memory_requirements.size,
      memory_type_index);

    const VkDeviceSize offset = 0;

    this->bind(memory, offset);
  }

  void bind(std::shared_ptr<VulkanMemory> memory, VkDeviceSize offset)
  {
    this->memory = std::move(memory);

    THROW_ON_ERROR(vkBindImageMemory(this->image->device->device,
                                     this->image->image,
                                     this->memory->memory,
                                     offset));
  }

  std::shared_ptr<VulkanImage> image;
  VkMemoryPropertyFlags memory_property_flags;
  std::shared_ptr<VulkanMemory> memory;
};

class DescriptorSetObject {
public:
  NO_COPY_OR_ASSIGNMENT(DescriptorSetObject)
  DescriptorSetObject() = delete;
  ~DescriptorSetObject() = default;

  DescriptorSetObject(const std::shared_ptr<VulkanDevice> & device, 
                      std::vector<VkWriteDescriptorSet> & write_descriptor_sets,
                      std::vector<VkDescriptorPoolSize> & descriptor_pool_sizes,
                      std::vector<VkDescriptorSetLayoutBinding> & descriptor_set_layout_bindings)
  {
    this->descriptor_set_layout = std::make_unique<VulkanDescriptorSetLayout>(device, descriptor_set_layout_bindings);
    this->descriptor_pool = std::make_unique<VulkanDescriptorPool>(device, descriptor_pool_sizes);
    this->descriptor_set = std::make_unique<VulkanDescriptorSets>(device, this->descriptor_pool, 1, descriptor_set_layout->layout);
    
    std::vector<VkDescriptorSetLayout> setlayouts { descriptor_set_layout->layout };
    this->pipeline_layout = std::make_unique<VulkanPipelineLayout>(device, setlayouts);

    for (auto & write_descriptor_set : write_descriptor_sets) {
      write_descriptor_set.dstSet = this->descriptor_set->descriptor_sets[0];
    }
    this->descriptor_set->update(write_descriptor_sets);
  }

  void bind(VkCommandBuffer command, VkPipelineBindPoint bind_point) const
  {
    this->descriptor_set->bind(command, bind_point, this->pipeline_layout->layout);
  }

  std::shared_ptr<VulkanDescriptorPool> descriptor_pool;
  std::shared_ptr<VulkanDescriptorSetLayout> descriptor_set_layout;
  std::shared_ptr<VulkanDescriptorSets> descriptor_set;
  std::shared_ptr<VulkanPipelineLayout> pipeline_layout;
};
