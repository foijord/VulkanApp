#pragma once

#include <Innovator/Defines.h>
#include <Innovator/Wrapper.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <memory>
#include <assert.h>

class BufferObject {
public:
  NO_COPY_OR_ASSIGNMENT(BufferObject)
  BufferObject() = delete;
  ~BufferObject() = default;
  
  BufferObject(std::shared_ptr<VulkanBuffer> buffer,
               VkMemoryPropertyFlags memory_property_flags) :
    buffer(std::move(buffer))
  {
    vkGetBufferMemoryRequirements(this->buffer->device->device,
                                  this->buffer->buffer,
                                  &this->memory_requirements);

    this->memory_type_index = this->buffer->device->physical_device.getMemoryTypeIndex(
      this->memory_requirements.memoryTypeBits,
      memory_property_flags);
  }

  void bind(std::shared_ptr<VulkanMemory> memory, VkDeviceSize offset)
  {
    this->memory = std::move(memory);
    this->offset = offset;

    THROW_ON_ERROR(vkBindBufferMemory(this->buffer->device->device,
                                      this->buffer->buffer,
                                      this->memory->memory,
                                      this->offset));
  }

  void memcpy(const void * data, size_t size) const
  {
    this->memory->memcpy(data, size, this->offset);
  }

  std::shared_ptr<VulkanBuffer> buffer;
  VkDeviceSize offset{ 0 };
  VkMemoryRequirements memory_requirements;
  uint32_t memory_type_index;
  std::shared_ptr<VulkanMemory> memory;
};

class ImageObject {
public:
  NO_COPY_OR_ASSIGNMENT(ImageObject)
  ImageObject() = delete;
  ~ImageObject() = default;

  ImageObject(std::shared_ptr<VulkanImage> image,
              VkMemoryPropertyFlags memory_property_flags) :
    image(std::move(image))
  {
    vkGetImageMemoryRequirements(this->image->device->device,
                                 this->image->image,
                                 &this->memory_requirements);

    this->memory_type_index = this->image->device->physical_device.getMemoryTypeIndex(
      this->memory_requirements.memoryTypeBits,
      memory_property_flags);
  }

  void bind(std::shared_ptr<VulkanMemory> memory, VkDeviceSize offset)
  {
    this->memory = std::move(memory);
    this->offset = offset;

    THROW_ON_ERROR(vkBindImageMemory(this->image->device->device,
                                     this->image->image,
                                     this->memory->memory,
                                     this->offset));
  }

  std::shared_ptr<VulkanImage> image;
  VkDeviceSize offset{ 0 };
  VkMemoryRequirements memory_requirements;
  uint32_t memory_type_index;
  std::shared_ptr<VulkanMemory> memory;
};
