#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <stdexcept>

class VkException : public std::runtime_error {
public:
  explicit VkException(const std::string & what, const VkResult result)
    : runtime_error(what), result(result) {}
  VkResult result;
};

class VkErrorOutOfDateException : public std::exception {};

#define THROW_ERROR(function)                                     \
{                                                                 \
	VkResult result = (function);                                   \
	if (result != VK_SUCCESS) {                                     \
    throw VkException("Vulkan API call failed", result);          \
	}                                                               \
}

namespace {
  std::vector<VkLayerProperties> EnumerateInstanceLayerProperties()
  {
    uint32_t properties_count;
    THROW_ERROR(vkEnumerateInstanceLayerProperties(&properties_count, nullptr));
    std::vector<VkLayerProperties> properties(properties_count);
    THROW_ERROR(vkEnumerateInstanceLayerProperties(&properties_count, properties.data()));
    return properties;
  }

  std::vector<VkExtensionProperties> EnumerateInstanceExtensionProperties()
  {
    uint32_t properties_count;
    THROW_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr));
    std::vector<VkExtensionProperties> properties(properties_count);
    THROW_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data()));
    return properties;
  }

  std::vector<VkLayerProperties> EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice)
  {
    uint32_t properties_count;
    THROW_ERROR(vkEnumerateDeviceLayerProperties(physicalDevice, &properties_count, nullptr));
    std::vector<VkLayerProperties> properties(properties_count);
    THROW_ERROR(vkEnumerateDeviceLayerProperties(physicalDevice, &properties_count, properties.data()));
    return properties;
  }

  std::vector<VkExtensionProperties> EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice)
  {
    uint32_t properties_count;
    THROW_ERROR(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &properties_count, nullptr));
    std::vector<VkExtensionProperties> properties(properties_count);
    THROW_ERROR(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &properties_count, properties.data()));
    return properties;
  }

  std::vector<VkPhysicalDevice> EnumeratePhysicalDevices(VkInstance instance)
  {
    uint32_t physical_device_count;
    THROW_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    THROW_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()));
    return physical_devices;
  }
}

class VulkanInstance {
public:
  VulkanInstance(const VkApplicationInfo & application_info,
                 const std::vector<const char*> & enabled_layer_names, 
                 const std::vector<const char*> & enabled_extension_names)
  {
    VkInstanceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layer_names.size());
    create_info.ppEnabledLayerNames = enabled_layer_names.data();
    create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extension_names.size());
    create_info.ppEnabledExtensionNames = enabled_extension_names.data();

    THROW_ERROR(vkCreateInstance(&create_info, nullptr, &this->instance));
  }

  ~VulkanInstance()
  {
    vkDestroyInstance(this->instance, nullptr);
  }

  VkInstance instance;
};

class VulkanDevice {
public:
  VulkanDevice(VkPhysicalDevice physical_device, 
               const VkPhysicalDeviceFeatures & enabled_features, 
               const std::vector<VkDeviceQueueCreateInfo> & queue_create_infos,
               const std::vector<const char*> & enabled_layer_names, 
               const std::vector<const char*> & enabled_extension_names)
  {
    VkDeviceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layer_names.size());
    create_info.ppEnabledLayerNames = enabled_layer_names.data();
    create_info.enabledExtensionCount = static_cast<uint32_t>(enabled_extension_names.size());
    create_info.ppEnabledExtensionNames = enabled_extension_names.data();
    create_info.pEnabledFeatures = &enabled_features;

    THROW_ERROR(vkCreateDevice(physical_device, &create_info, nullptr, &this->device));

    vkGetPhysicalDeviceMemoryProperties(physical_device, &this->memory_properties);
  }

  ~VulkanDevice()
  {
    vkDestroyDevice(this->device, nullptr);
  }

  void waitIdle() 
  {
    vkDeviceWaitIdle(this->device);
  }

  VkDevice device;
  VkPhysicalDeviceMemoryProperties memory_properties;
};

class VulkanCommandPool {
public:
  VulkanCommandPool(const std::shared_ptr<VulkanDevice> & device, VkCommandPoolCreateFlags flags, uint32_t queue_family_index)
    : device(device)
  {
    VkCommandPoolCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = flags;
    create_info.queueFamilyIndex = queue_family_index;

    THROW_ERROR(vkCreateCommandPool(this->device->device, &create_info, nullptr, &this->pool));
  }

  ~VulkanCommandPool()
  {
    vkDestroyCommandPool(this->device->device, this->pool, nullptr);
  }

  VkCommandPool pool;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanDescriptorPool {
public:
  VulkanDescriptorPool(const std::shared_ptr<VulkanDevice> & device, std::vector<VkDescriptorPoolSize> descriptor_pool_sizes)
    : device(device)
  {
    VkDescriptorPoolCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    create_info.maxSets = 1;
    create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
    create_info.pPoolSizes = descriptor_pool_sizes.data();

    THROW_ERROR(vkCreateDescriptorPool(this->device->device, &create_info, nullptr, &this->pool));
  }

  ~VulkanDescriptorPool()
  {
    vkDestroyDescriptorPool(this->device->device, this->pool, nullptr);
  }

  VkDescriptorPool pool;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanDescriptorSetLayout {
public:
  VulkanDescriptorSetLayout(const std::shared_ptr<VulkanDevice> & device, std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings)
    : device(device)
  {
    VkDescriptorSetLayoutCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.bindingCount = static_cast<uint32_t>(descriptor_set_layout_bindings.size());
    create_info.pBindings = descriptor_set_layout_bindings.data();

    THROW_ERROR(vkCreateDescriptorSetLayout(this->device->device, &create_info, nullptr, &this->layout));
  }

  ~VulkanDescriptorSetLayout()
  {
    vkDestroyDescriptorSetLayout(this->device->device, this->layout, nullptr);
  }

  VkDescriptorSetLayout layout;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanDescriptorSets {
public:
  VulkanDescriptorSets(const std::shared_ptr<VulkanDevice> & device,
                       const std::shared_ptr<VulkanDescriptorPool> & pool,
                       uint32_t descriptor_set_count,
                       VkDescriptorSetLayout descriptor_set_layout)
    : device(device), pool(pool)
  {
    VkDescriptorSetAllocateInfo allocate_info;
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.descriptorPool = this->pool->pool;
    allocate_info.descriptorSetCount = descriptor_set_count;
    allocate_info.pSetLayouts = &descriptor_set_layout;

    this->descriptor_sets.resize(descriptor_set_count);
    THROW_ERROR(vkAllocateDescriptorSets(this->device->device, &allocate_info, this->descriptor_sets.data()));
  }

  ~VulkanDescriptorSets()
  {
    vkFreeDescriptorSets(this->device->device, 
                         this->pool->pool, 
                         static_cast<uint32_t>(this->descriptor_sets.size()), 
                         this->descriptor_sets.data());
  }

  void update(const std::vector<VkWriteDescriptorSet> & descriptor_writes, 
              const std::vector<VkCopyDescriptorSet> & descriptor_copies = std::vector<VkCopyDescriptorSet>())
  {
    vkUpdateDescriptorSets(this->device->device, 
                           static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(),
                           static_cast<uint32_t>(descriptor_copies.size()), descriptor_copies.data());
  }

  void bind(VkCommandBuffer buffer, VkPipelineBindPoint bind_point, VkPipelineLayout layout)
  {
    vkCmdBindDescriptorSets(buffer, bind_point, layout, 0, static_cast<uint32_t>(this->descriptor_sets.size()), this->descriptor_sets.data(), 0, nullptr);
  }

  std::vector<VkDescriptorSet> descriptor_sets;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanDescriptorPool> pool;
};


class VulkanCommandBuffers {
public:
  VulkanCommandBuffers(const std::shared_ptr<VulkanDevice> & device, 
                       const std::shared_ptr<VulkanCommandPool> & pool, 
                       size_t command_buffer_count = 1, 
                       VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    : device(device), pool(pool)
  {
    this->buffers.resize(command_buffer_count);

    VkCommandBufferAllocateInfo allocate_info;
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.commandPool = pool->pool;
    allocate_info.level = level;
    allocate_info.commandBufferCount = static_cast<uint32_t>(this->buffers.size());

    THROW_ERROR(vkAllocateCommandBuffers(this->device->device, &allocate_info, this->buffers.data()));
  }

  ~VulkanCommandBuffers()
  {
    vkFreeCommandBuffers(this->device->device, this->pool->pool, static_cast<uint32_t>(this->buffers.size()), this->buffers.data());
  }

  void begin(size_t buffer_index = 0, 
             VkRenderPass renderpass = nullptr, 
             uint32_t subpass = 0,
             VkFramebuffer framebuffer = nullptr, 
             VkCommandBufferUsageFlags flags = 0)
  {
    VkCommandBufferInheritanceInfo inheritance_info;
    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritance_info.pNext = nullptr;
    inheritance_info.renderPass = renderpass;
    inheritance_info.subpass = subpass;
    inheritance_info.framebuffer = framebuffer;
    inheritance_info.occlusionQueryEnable = VK_FALSE;
    inheritance_info.queryFlags = 0;
    inheritance_info.pipelineStatistics = 0;

    VkCommandBufferBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = flags;
    begin_info.pInheritanceInfo = &inheritance_info;

    THROW_ERROR(vkBeginCommandBuffer(this->buffers[buffer_index], &begin_info));
  }

  void end(size_t buffer_index = 0)
  {
    THROW_ERROR(vkEndCommandBuffer(this->buffers[buffer_index]));
  }

  void submit(VkQueue queue, 
              VkPipelineStageFlags flags, 
              size_t buffer_index, 
              const std::vector<VkSemaphore> & wait_semaphores = std::vector<VkSemaphore>(),
              const std::vector<VkSemaphore> & signal_semaphores = std::vector<VkSemaphore>(),
              VkFence fence = VK_NULL_HANDLE)
  {
    VulkanCommandBuffers::submit(queue, flags, { this->buffers[buffer_index] }, wait_semaphores, signal_semaphores, fence);
  }

  void submit(VkQueue queue,
              VkPipelineStageFlags flags,
              const std::vector<VkSemaphore> & wait_semaphores = std::vector<VkSemaphore>(),
              const std::vector<VkSemaphore> & signal_semaphores = std::vector<VkSemaphore>(),
              VkFence fence = VK_NULL_HANDLE)
  {
    VulkanCommandBuffers::submit(queue, flags, this->buffers, wait_semaphores, signal_semaphores, fence);
  }

  static void submit(VkQueue queue, 
                     VkPipelineStageFlags flags,
                     const std::vector<VkCommandBuffer> & buffers, 
                     const std::vector<VkSemaphore> & wait_semaphores,
                     const std::vector<VkSemaphore> & signal_semaphores,
                     VkFence fence)
  {
    VkSubmitInfo submit_info;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = static_cast<uint32_t>(wait_semaphores.size());
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = &flags;
    submit_info.commandBufferCount = static_cast<uint32_t>(buffers.size());
    submit_info.pCommandBuffers = buffers.data();
    submit_info.signalSemaphoreCount = static_cast<uint32_t>(signal_semaphores.size());
    submit_info.pSignalSemaphores = signal_semaphores.data();

    THROW_ERROR(vkQueueSubmit(queue, 1, &submit_info, fence));
  }

  VkCommandBuffer buffer(size_t buffer_index = 0)
  {
    return this->buffers[buffer_index];
  }

  std::vector<VkCommandBuffer> buffers;

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanCommandPool> pool;
};

namespace {
  VkMemoryRequirements get_memory_requirements(VkDevice device, VkImage image)
  {
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device, image, &memory_requirements);
    return memory_requirements;
  }

  VkMemoryRequirements get_memory_requirements(VkDevice device, VkBuffer buffer)
  {
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);
    return memory_requirements;
  }

  uint32_t get_memory_type_index(VkPhysicalDeviceMemoryProperties properties, VkMemoryRequirements requirements, VkMemoryPropertyFlags flags)
  {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
      if (((requirements.memoryTypeBits >> i) & 1) == 1)
        if ((properties.memoryTypes[i].propertyFlags & flags) == flags)
          return i;
    }
    throw std::runtime_error("VulkanMemory: could not find suitable memory type");
  };
}

class VulkanMemory {
public:
  VulkanMemory(const std::shared_ptr<VulkanDevice> & device, VkMemoryRequirements requirements, VkMemoryPropertyFlags flags)
    : device(device)
  {
    VkAllocationCallbacks allocation_callbacks;
    allocation_callbacks.pUserData = nullptr;
    allocation_callbacks.pfnAllocation = (PFN_vkAllocationFunction)&VulkanMemory::allocate;
    allocation_callbacks.pfnReallocation = (PFN_vkReallocationFunction)&VulkanMemory::reallocate;
    allocation_callbacks.pfnFree = (PFN_vkFreeFunction)&VulkanMemory::free;
    allocation_callbacks.pfnInternalAllocation = nullptr;
    allocation_callbacks.pfnInternalFree = nullptr;

    VkMemoryAllocateInfo allocate_info;
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = get_memory_type_index(this->device->memory_properties, requirements, flags);

    THROW_ERROR(vkAllocateMemory(this->device->device, &allocate_info, &allocation_callbacks, &this->memory));
  }

  VulkanMemory(const std::shared_ptr<VulkanDevice> & device, VkImage image, VkMemoryPropertyFlags flags)
    : VulkanMemory(device, get_memory_requirements(device->device, image), flags)
  {
    THROW_ERROR(vkBindImageMemory(this->device->device, image, this->memory, 0));
  }

  VulkanMemory(const std::shared_ptr<VulkanDevice> & device, VkBuffer buffer, VkMemoryPropertyFlags flags)
    : VulkanMemory(device, get_memory_requirements(device->device, buffer), flags)
  {
    THROW_ERROR(vkBindBufferMemory(this->device->device, buffer, this->memory, 0));
  }

  ~VulkanMemory()
  {
    vkFreeMemory(this->device->device, this->memory, nullptr);
  }

  static void * allocate(void * userdata, size_t size, size_t alignment, VkSystemAllocationScope scope)
  {
    return _aligned_malloc(size, alignment);
  }

  static void free(void * userdata, void * block)
  {
    _aligned_free(block);
  }

  static void * reallocate(void * userdata, void * block, size_t size, size_t alignment, VkSystemAllocationScope scope)
  {
    return _aligned_realloc(block, size, alignment);
  }

  void * map(size_t size)
  {
    void * data;
    THROW_ERROR(vkMapMemory(this->device->device, this->memory, 0, size, 0, &data));
    return data;
  }

  void unmap()
  {
    vkUnmapMemory(this->device->device, this->memory);
  }

  void memcpy(const void * data, size_t size)
  {
    ::memcpy(this->map(size), data, size);
    this->unmap();
  }

  VkDeviceMemory memory;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanImage {
public:
  VulkanImage(const std::shared_ptr<VulkanDevice> & device, 
              VkImageCreateFlags flags,
              VkImageType image_type, 
              VkFormat format, 
              VkExtent3D extent,
              uint32_t mip_levels,
              uint32_t array_layers,
              VkSampleCountFlagBits samples,
              VkImageTiling tiling, 
              VkImageUsageFlags usage, 
              VkSharingMode sharing_mode,
              std::vector<uint32_t> queue_family_indices,
              VkImageLayout initial_layout)
    : device(device)
  {
    VkImageCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = flags;
    create_info.imageType = image_type;
    create_info.format = format;
    create_info.extent = extent;
    create_info.mipLevels = mip_levels;
    create_info.arrayLayers = array_layers;
    create_info.samples = samples;
    create_info.tiling = tiling;
    create_info.usage = usage;
    create_info.sharingMode = sharing_mode;
    create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
    create_info.pQueueFamilyIndices = queue_family_indices.data();
    create_info.initialLayout = initial_layout;

    THROW_ERROR(vkCreateImage(this->device->device, &create_info, nullptr, &this->image));
  }

  ~VulkanImage()
  {
    vkDestroyImage(this->device->device, this->image, nullptr);
  }

  VkImage image;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanImageView {
public:
  VulkanImageView(const std::shared_ptr<VulkanDevice> & device,
                  VkImage image, 
                  VkFormat format, 
                  VkImageViewType view_type, 
                  VkComponentMapping components,
                  VkImageSubresourceRange subresource_range)
    : device(device)
  {
    VkImageViewCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use
    create_info.image = image;
    create_info.viewType = view_type;
    create_info.format = format;
    create_info.components = components;
    create_info.subresourceRange = subresource_range;

    THROW_ERROR(vkCreateImageView(this->device->device, &create_info, nullptr, &this->view));
  }


  ~VulkanImageView()
  {
    vkDestroyImageView(this->device->device, this->view, nullptr);
  }

  VkImageView view;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanSampler {
public:
  VulkanSampler(const std::shared_ptr<VulkanDevice> & device, 
                VkFilter magFilter,
                VkFilter minFilter,
                VkSamplerMipmapMode mipmapMode,
                VkSamplerAddressMode addressModeU,
                VkSamplerAddressMode addressModeV,
                VkSamplerAddressMode addressModeW,
                float mipLodBias,
                VkBool32 anisotropyEnable,
                float maxAnisotropy,
                VkBool32 compareEnable,
                VkCompareOp compareOp,
                float minLod,
                float maxLod,
                VkBorderColor borderColor,
                VkBool32 unnormalizedCoordinates)
    : device(device)
  {
    VkSamplerCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use
    create_info.magFilter = magFilter;
    create_info.minFilter = minFilter;
    create_info.mipmapMode = mipmapMode;
    create_info.addressModeU = addressModeU;
    create_info.addressModeV = addressModeV;
    create_info.addressModeW = addressModeW;
    create_info.mipLodBias = mipLodBias;
    create_info.anisotropyEnable = anisotropyEnable;
    create_info.maxAnisotropy = maxAnisotropy;
    create_info.compareEnable = compareEnable;
    create_info.compareOp = compareOp;
    create_info.minLod = minLod;
    create_info.maxLod = maxLod;
    create_info.borderColor = borderColor;
    create_info.unnormalizedCoordinates = unnormalizedCoordinates;

    THROW_ERROR(vkCreateSampler(this->device->device, &create_info, nullptr, &this->sampler));
  }

  ~VulkanSampler()
  {
    vkDestroySampler(this->device->device, this->sampler, nullptr);
  }

  VkSampler sampler;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanBuffer {
public:
  VulkanBuffer(const std::shared_ptr<VulkanDevice> & device,
               VkBufferCreateFlags flags, 
               VkDeviceSize size, 
               VkBufferUsageFlags usage,
               VkSharingMode sharingMode,
               const std::vector<uint32_t> & queueFamilyIndices)
    : device(device)
  {
    VkBufferCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = flags;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = sharingMode;
    create_info.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    create_info.pQueueFamilyIndices = queueFamilyIndices.data();

    THROW_ERROR(vkCreateBuffer(this->device->device, &create_info, nullptr, &this->buffer));
  }

  ~VulkanBuffer()
  {
    vkDestroyBuffer(this->device->device, this->buffer, nullptr);
  }

  VkBuffer buffer;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanSemaphore {
public:
  VulkanSemaphore(const std::shared_ptr<VulkanDevice> & device)
    : device(device)
  {
    VkSemaphoreCreateInfo create_info;
    ::memset(&create_info, 0, sizeof(VkSemaphoreCreateInfo));
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use

    THROW_ERROR(vkCreateSemaphore(this->device->device, &create_info, nullptr, &this->semaphore));
  }

  ~VulkanSemaphore()
  {
    vkDestroySemaphore(this->device->device, this->semaphore, nullptr);
  }

  VkSemaphore semaphore;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanShaderModule {
public:
  VulkanShaderModule(const std::shared_ptr<VulkanDevice> & device, 
                     const std::vector<char> & code)
    : device(device)
  {
    VkShaderModuleCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    THROW_ERROR(vkCreateShaderModule(this->device->device, &create_info, nullptr, &this->module));
  }

  ~VulkanShaderModule()
  {
    vkDestroyShaderModule(this->device->device, this->module, nullptr);
  }

  VkShaderModule module;
  std::shared_ptr<VulkanDevice> device;
};


class VulkanPipelineCache {
public:
  VulkanPipelineCache(const std::shared_ptr<VulkanDevice> & device)
    : device(device)
  {
    VkPipelineCacheCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use
    create_info.initialDataSize = 0;
    create_info.pInitialData = nullptr;

    THROW_ERROR(vkCreatePipelineCache(this->device->device, &create_info, nullptr, &this->cache));
  }

  ~VulkanPipelineCache()
  {
    vkDestroyPipelineCache(this->device->device, this->cache, nullptr);
  }

  VkPipelineCache cache;
  std::shared_ptr<VulkanDevice> device;
};


class VulkanRenderPass {
public:
  VulkanRenderPass(const std::shared_ptr<VulkanDevice> & device,
                   const std::vector<VkAttachmentDescription> & attachments,
                   const std::vector<VkSubpassDescription> & subpasses,
                   const std::vector<VkSubpassDependency> & dependencies = std::vector<VkSubpassDependency>())
    : device(device)
  {
    VkRenderPassCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use
    create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    create_info.pAttachments = attachments.data();
    create_info.subpassCount = static_cast<uint32_t>(subpasses.size());
    create_info.pSubpasses = subpasses.data();
    create_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    create_info.pDependencies = dependencies.data();

    THROW_ERROR(vkCreateRenderPass(this->device->device, &create_info, nullptr, &this->render_pass));
  }

  ~VulkanRenderPass()
  {
    vkDestroyRenderPass(this->device->device, this->render_pass, nullptr);
  }

  void begin(VkCommandBuffer commandbuffer, 
             VkFramebuffer framebuffer, 
             VkRect2D renderArea, 
             std::vector<VkClearValue> clearvalues, 
             VkSubpassContents contents)
  {
    VkRenderPassBeginInfo begin_info;
    begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.renderPass = this->render_pass;
    begin_info.framebuffer = framebuffer;
    begin_info.renderArea = renderArea;
    begin_info.clearValueCount = static_cast<uint32_t>(clearvalues.size());
    begin_info.pClearValues = clearvalues.data();

    vkCmdBeginRenderPass(commandbuffer, &begin_info, contents);
  }

  void end(VkCommandBuffer commandbuffer)
  {
    vkCmdEndRenderPass(commandbuffer);
  }

  VkRenderPass render_pass;
  std::shared_ptr<VulkanDevice> device;
};


class VulkanFramebuffer {
public:
  VulkanFramebuffer(const std::shared_ptr<VulkanDevice> & device,
                    VkRenderPass renderPass,
                    std::vector<VkImageView> attachments, 
                    uint32_t width,
                    uint32_t height,
                    uint32_t layers)
    : device(device)
  {
    VkFramebufferCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.renderPass = renderPass;
    create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    create_info.pAttachments = attachments.data();
    create_info.width = width;
    create_info.height = height;
    create_info.layers = layers;

    THROW_ERROR(vkCreateFramebuffer(this->device->device, &create_info, nullptr, &this->framebuffer));
  }

  ~VulkanFramebuffer()
  {
    vkDestroyFramebuffer(this->device->device, this->framebuffer, nullptr);
  }

  VkFramebuffer framebuffer;
  std::shared_ptr<VulkanDevice> device;
};


class VulkanPipelineLayout {
public:
  VulkanPipelineLayout(const std::shared_ptr<VulkanDevice> & device, 
                       const std::vector<VkDescriptorSetLayout> & setlayouts,
                       const std::vector<VkPushConstantRange> & pushconstantranges = std::vector<VkPushConstantRange>())
    : device(device)
  {
    VkPipelineLayoutCreateInfo create_info;
    ::memset(&create_info, 0, sizeof(VkPipelineLayoutCreateInfo));
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use
    create_info.setLayoutCount = static_cast<uint32_t>(setlayouts.size());
    create_info.pSetLayouts = setlayouts.data();
    create_info.pushConstantRangeCount = static_cast<uint32_t>(pushconstantranges.size());
    create_info.pPushConstantRanges = pushconstantranges.data();

    THROW_ERROR(vkCreatePipelineLayout(this->device->device, &create_info, nullptr, &this->layout));
  }

  ~VulkanPipelineLayout()
  {
    vkDestroyPipelineLayout(this->device->device, this->layout, nullptr);
  }

  VkPipelineLayout layout;
  std::shared_ptr<VulkanDevice> device;
};


class VulkanComputePipeline {
public:
  VulkanComputePipeline(const std::shared_ptr<VulkanDevice> & device,
                        VkPipelineCache pipelineCache,
                        VkPipelineCreateFlags flags,
                        const VkPipelineShaderStageCreateInfo & stage,
                        VkPipelineLayout layout,
                        VkPipeline basePipelineHandle = nullptr,
                        int32_t basePipelineIndex = 0)
    : device(device)
  {
    VkComputePipelineCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = flags;
    create_info.stage = stage;
    create_info.layout = layout;
    create_info.basePipelineHandle = basePipelineHandle;
    create_info.basePipelineIndex = basePipelineIndex;

    THROW_ERROR(vkCreateComputePipelines(this->device->device, pipelineCache, 1, &create_info, nullptr, &this->pipeline));
  }

  ~VulkanComputePipeline()
  {
    vkDestroyPipeline(this->device->device, this->pipeline, nullptr);
  }

  void bind(VkCommandBuffer buffer)
  {
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->pipeline);
  }

  VkPipeline pipeline;
  std::shared_ptr<VulkanDevice> device;
};
