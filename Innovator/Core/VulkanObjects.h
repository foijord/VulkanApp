#pragma once

#include <Innovator/core/Vulkan/Wrapper.h>

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <gli/texture.hpp>

#include <map>
#include <vector>
#include <string>
#include <memory>

class VulkanMemory;

namespace {
  template<class PropertyType>
  std::vector<std::string> extract_layer_names(std::vector<PropertyType> properties)
  {
    std::vector<std::string> layer_names;
    std::transform(properties.begin(), properties.end(), std::back_inserter(layer_names), [](auto p) { return p.layerName; });
    return layer_names;
  }

  template<class PropertyType>
  std::vector<std::string> extract_extension_names(std::vector<PropertyType> properties)
  {
    std::vector<std::string> exension_names;
    std::transform(properties.begin(), properties.end(), std::back_inserter(exension_names), [](auto p) { return p.extensionName; });
    return exension_names;
  }

  std::vector<std::string> get_set_difference(std::vector<std::string> required, std::vector<std::string> supported)
  {
    std::sort(required.begin(), required.end());
    std::sort(supported.begin(), supported.end());

    std::vector<std::string> difference;
    std::set_difference(required.begin(), required.end(), supported.begin(), supported.end(), std::back_inserter(difference));
    return difference;
  }

  std::vector<const char*> get_instance_layer_names(const std::vector<std::string> & instance_layers)
  {
    std::vector<std::string> supported_instance_layer_names = extract_layer_names(EnumerateInstanceLayerProperties());
    std::vector<std::string> unsupported_layers = get_set_difference(instance_layers, supported_instance_layer_names);
    if (!unsupported_layers.empty()) {
      throw std::runtime_error("unsupported instance layers");
    }
    std::vector<const char*> instance_layer_names;
    for (const std::string & name : instance_layers) {
      instance_layer_names.push_back(name.c_str());
    }
    return instance_layer_names;
  }

  std::vector<const char*> get_instance_extension_names(const std::vector<std::string> & instance_extensions)
  {
    std::vector<std::string> supported_instance_extension_names = extract_extension_names(EnumerateInstanceExtensionProperties());
    std::vector<std::string> unsupported_extensions = get_set_difference(instance_extensions, supported_instance_extension_names);
    if (!unsupported_extensions.empty()) {
      throw std::runtime_error("unsupported instance extensions");
    }
    std::vector<const char*> instance_extension_names;
    for (const std::string & name : instance_extensions) {
      instance_extension_names.push_back(name.c_str());
    }
    return instance_extension_names;
  }

  std::vector<const char*> get_device_layer_names(const std::vector<std::string> & device_layers, VkPhysicalDevice physical_device)
  {
    std::vector<std::string> supported_device_layer_names = extract_layer_names(EnumerateDeviceLayerProperties(physical_device));
    std::vector<std::string> unsupported_layers = get_set_difference(device_layers, supported_device_layer_names);
    if (!unsupported_layers.empty()) {
      throw std::runtime_error("unsupported device layers");
    }
    std::vector<const char*> device_layer_names;
    for (const std::string & name : device_layers) {
      device_layer_names.push_back(name.c_str());
    }
    return device_layer_names;
  }

  std::vector<const char*> get_device_extension_names(const std::vector<std::string> & device_extensions, VkPhysicalDevice physical_device)
  {
    std::vector<std::string> supported_device_extension_names = extract_extension_names(EnumerateDeviceExtensionProperties(physical_device));
    std::vector<std::string> unsupported_extensions = get_set_difference(device_extensions, supported_device_extension_names);
    if (!unsupported_extensions.empty()) {
      throw std::runtime_error("unsupported device extensions");
    }
    std::vector<const char*> device_extension_names;
    for (const std::string & name : device_extensions) {
      device_extension_names.push_back(name.c_str());
    }
    return device_extension_names;
  }
}

class Vulkan {
public:
  Vulkan(const VkApplicationInfo & application_info, const std::vector<std::string> & instance_layers, const std::vector<std::string> & instance_extensions)
    : instance(std::make_unique<VulkanInstance>(application_info, get_instance_layer_names(instance_layers), get_instance_extension_names(instance_extensions))),
      physical_devices(EnumeratePhysicalDevices(this->instance->instance)),
      vkQueuePresent((PFN_vkQueuePresentKHR)vkGetInstanceProcAddr(this->instance->instance, "vkQueuePresentKHR")),
      vkCreateSwapchain((PFN_vkCreateSwapchainKHR)vkGetInstanceProcAddr(this->instance->instance, "vkCreateSwapchainKHR")),
      vkAcquireNextImage((PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(this->instance->instance, "vkAcquireNextImageKHR")),
      vkDestroySwapchain((PFN_vkDestroySwapchainKHR)vkGetInstanceProcAddr(this->instance->instance, "vkDestroySwapchainKHR")),
      vkGetSwapchainImages((PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(this->instance->instance, "vkGetSwapchainImagesKHR")),
      vkCreateDebugReportCallback((PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(this->instance->instance, "vkCreateDebugReportCallbackEXT")),
      vkDestroyDebugReportCallback((PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(this->instance->instance, "vkDestroyDebugReportCallbackEXT")),
      vkGetPhysicalDeviceSurfaceSupport((PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(this->instance->instance, "vkGetPhysicalDeviceSurfaceSupportKHR")),
      vkGetPhysicalDeviceSurfaceFormats((PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(this->instance->instance, "vkGetPhysicalDeviceSurfaceFormatsKHR")),
      vkGetPhysicalDeviceSurfaceCapabilities((PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(this->instance->instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR")),
      vkGetPhysicalDeviceSurfacePresentModes((PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(this->instance->instance, "vkGetPhysicalDeviceSurfacePresentModesKHR"))
  {
    if (!(this->vkQueuePresent &&
          this->vkCreateSwapchain &&
          this->vkAcquireNextImage &&
          this->vkDestroySwapchain &&
          this->vkGetSwapchainImages &&
          this->vkCreateDebugReportCallback &&
          this->vkDestroyDebugReportCallback &&
          this->vkGetPhysicalDeviceSurfaceSupport &&
          this->vkGetPhysicalDeviceSurfaceFormats &&
          this->vkGetPhysicalDeviceSurfaceCapabilities &&
          this->vkGetPhysicalDeviceSurfacePresentModes))
    {
      throw std::runtime_error("Vulkan::Vulkan(): vkGetInstanceProcAddr failed.");
    }
  }

  ~Vulkan() {}

  std::unique_ptr<VulkanInstance> instance;
  std::vector<VkPhysicalDevice> physical_devices;

  PFN_vkQueuePresentKHR vkQueuePresent;
  PFN_vkCreateSwapchainKHR vkCreateSwapchain;
  PFN_vkAcquireNextImageKHR vkAcquireNextImage;
  PFN_vkDestroySwapchainKHR vkDestroySwapchain;
  PFN_vkGetSwapchainImagesKHR vkGetSwapchainImages;
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallback;
  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallback;
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupport;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormats;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilities;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModes;
};

//*******************************************************************************************************************************
class Device {
public:
  Device(const std::vector<std::string> & device_layers, 
         const std::vector<std::string> & device_extensions,
         VkPhysicalDevice physical_device, 
         uint32_t queue_index)
    : physical_device(physical_device), queue_index(queue_index)
  {
    VkPhysicalDeviceFeatures enabled_features;
    ::memset(&enabled_features, 0, sizeof(VkPhysicalDeviceFeatures));
    enabled_features.textureCompressionBC = VK_TRUE;

    float queue_priorities[1] = { 0.0 };
    VkDeviceQueueCreateInfo queue_create_info;
    ::memset(&queue_create_info, 0, sizeof(VkDeviceQueueCreateInfo));
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = queue_priorities;
    queue_create_info.queueFamilyIndex = queue_index;

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.push_back(queue_create_info);

    this->device = std::make_unique<VulkanDevice>(
      physical_device,
      enabled_features,
      queue_create_infos,
      get_device_layer_names(device_layers, physical_device),
      get_device_extension_names(device_extensions, physical_device));

    this->command_pool = std::make_unique<VulkanCommandPool>(this->device, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, this->queue_index);

    vkGetDeviceQueue(this->device->device, this->queue_index, 0, &this->queue);
    vkGetPhysicalDeviceMemoryProperties(this->physical_device, &this->memory_properties);
  }

  ~Device() {}

  void waitIdle()
  {
    this->device->waitIdle();
  }

  VkQueue queue;
  uint32_t queue_index;
  VkPhysicalDevice physical_device;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanCommandPool> command_pool;
  VkPhysicalDeviceMemoryProperties memory_properties;
};

class VulkanSwapchain {
public:
  VulkanSwapchain(const std::shared_ptr<Vulkan> & vulkan,
                  const std::shared_ptr<VulkanDevice> & device,
                  VkSurfaceKHR surface,
                  uint32_t minImageCount,
                  VkFormat imageFormat,
                  VkColorSpaceKHR imageColorSpace,
                  VkExtent2D imageExtent,
                  uint32_t imageArrayLayers,
                  VkImageUsageFlags imageUsage,
                  VkSharingMode imageSharingMode,
                  std::vector<uint32_t> queueFamilyIndices,
                  VkSurfaceTransformFlagBitsKHR preTransform,
                  VkCompositeAlphaFlagBitsKHR compositeAlpha,
                  VkPresentModeKHR presentMode,
                  VkBool32 clipped,
                  VkSwapchainKHR oldSwapchain)
    : vulkan(vulkan), device(device)
  {
    VkSwapchainCreateInfoKHR create_info;
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0; // reserved for future use
    create_info.surface = surface;
    create_info.minImageCount = minImageCount;
    create_info.imageFormat = imageFormat;
    create_info.imageColorSpace = imageColorSpace;
    create_info.imageExtent = imageExtent;
    create_info.imageArrayLayers = imageArrayLayers;
    create_info.imageUsage = imageUsage;
    create_info.imageSharingMode = imageSharingMode;
    create_info.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    create_info.pQueueFamilyIndices = queueFamilyIndices.data();
    create_info.preTransform = preTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    create_info.clipped = clipped;
    create_info.oldSwapchain = oldSwapchain;

    THROW_ERROR(this->vulkan->vkCreateSwapchain(this->device->device, &create_info, nullptr, &this->swapchain));
  }

  ~VulkanSwapchain()
  {
    this->vulkan->vkDestroySwapchain(this->device->device, this->swapchain, nullptr);
  }

  std::vector<VkImage> getSwapchainImages()
  {
    uint32_t imagecount;
    THROW_ERROR(this->vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &imagecount, nullptr));
    std::vector<VkImage> images(imagecount);
    THROW_ERROR(this->vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &imagecount, images.data()));
    return images;
  }

  uint32_t acquireNextImage(VkSemaphore semaphore)
  {
    uint32_t image_index;
    VkResult result = this->vulkan->vkAcquireNextImage(this->device->device, this->swapchain, UINT64_MAX, semaphore, 0, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      throw VkErrorOutOfDateException();
    }
    if (result != VK_SUCCESS) {
      throw std::runtime_error("vkAcquireNextImage failed.");
    }
    return image_index;
  }

  void present(VkQueue queue, uint32_t image_index)
  {
    VkPresentInfoKHR present_info;
    ::memset(&present_info, 0, sizeof(VkPresentInfoKHR));
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &this->swapchain;
    present_info.pImageIndices = &image_index;

    THROW_ERROR(this->vulkan->vkQueuePresent(queue, &present_info));
    THROW_ERROR(vkQueueWaitIdle(queue));
  }

  VkSwapchainKHR swapchain;
  std::shared_ptr<Vulkan> vulkan;
  std::shared_ptr<VulkanDevice> device;
};

//*******************************************************************************************************************************
class VulkanDebugCallback {
public:
  VulkanDebugCallback(const std::shared_ptr<Vulkan> & vulkan, 
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

    THROW_ERROR(this->vulkan->vkCreateDebugReportCallback(this->vulkan->instance->instance, &create_info, nullptr, &this->callback));
  }

  ~VulkanDebugCallback()
  {
    this->vulkan->vkDestroyDebugReportCallback(this->vulkan->instance->instance, this->callback, nullptr);
  }

  VkDebugReportCallbackEXT callback;
  std::shared_ptr<Vulkan> vulkan;
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
      this->image->image,
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
    this->staging_memory = std::make_unique<VulkanMemory>(this->device, this->staging_buffer->buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
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
        memory(std::make_unique<VulkanMemory>(device, this->buffer->buffer, property_flags))
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

class VulkanShaderModuleDescription {
public:
  VkShaderStageFlagBits stage;
  VkShaderModule module;
};

class SwapchainObject {
public:
  SwapchainObject(const std::shared_ptr<Vulkan> & vulkan, 
                  const std::shared_ptr<Device> & device,
                  VulkanCommandBuffers * staging_command, 
                  const std::unique_ptr<ImageObject> & image, VkSurfaceKHR surface, 
                  VkSurfaceCapabilitiesKHR surface_capabilities, 
                  VkSurfaceFormatKHR surface_format)
    : device(device), semaphore(std::make_unique<VulkanSemaphore>(device->device))
  {
    this->swapchain = std::make_unique<VulkanSwapchain>(
      vulkan,
      this->device->device,
      surface,
      3,
      surface_format.format,
      surface_format.colorSpace,
      surface_capabilities.currentExtent,
      1,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>(),
      surface_capabilities.currentTransform,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      VK_PRESENT_MODE_MAILBOX_KHR,
      VK_FALSE,
      nullptr);

    std::vector<VkImage> swapchain_images = this->swapchain->getSwapchainImages();
    this->commands = std::make_unique<VulkanCommandBuffers>(this->device->device, device->command_pool, swapchain_images.size());

    VkImageMemoryBarrier default_memory_barrier;
    ::memset(&default_memory_barrier, 0, sizeof(VkImageMemoryBarrier));
    default_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    default_memory_barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    default_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    default_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    default_memory_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    for (uint32_t i = 0; i < swapchain_images.size(); i++) {
      staging_command->begin();

      default_memory_barrier.image = swapchain_images[i];

      vkCmdPipelineBarrier(staging_command->buffer(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        1, &default_memory_barrier);

      staging_command->end();
      staging_command->submit(this->device->queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
      THROW_ERROR(vkQueueWaitIdle(this->device->queue));
    }

    std::vector<VkImageMemoryBarrier> src_image_barriers = {
      default_memory_barrier,
      default_memory_barrier,
    };

    src_image_barriers[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    src_image_barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    src_image_barriers[0].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    src_image_barriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    src_image_barriers[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    src_image_barriers[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    src_image_barriers[1].oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    src_image_barriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    std::vector<VkImageMemoryBarrier> dst_image_barriers = src_image_barriers;
    std::swap(dst_image_barriers[0].oldLayout, dst_image_barriers[0].newLayout);
    std::swap(dst_image_barriers[1].oldLayout, dst_image_barriers[1].newLayout);
    std::swap(dst_image_barriers[0].srcAccessMask, dst_image_barriers[0].dstAccessMask);
    std::swap(dst_image_barriers[1].srcAccessMask, dst_image_barriers[1].dstAccessMask);

    for (uint32_t i = 0; i < swapchain_images.size(); i++) {

      src_image_barriers[1].image = image->image->image;
      dst_image_barriers[1].image = image->image->image;
      src_image_barriers[0].image = swapchain_images[i];
      dst_image_barriers[0].image = swapchain_images[i];

      CommandBufferRecorder buffer_scope(this->commands.get(), i);

      vkCmdPipelineBarrier(buffer_scope.command(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(src_image_barriers.size()),
        src_image_barriers.data());

      image->copyImageToImage(buffer_scope.command(), swapchain_images[i]);

      vkCmdPipelineBarrier(buffer_scope.command(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        0, 0, nullptr, 0, nullptr,
        static_cast<uint32_t>(dst_image_barriers.size()),
        dst_image_barriers.data());
    }
  }

  ~SwapchainObject() {}

  void swapBuffers()
  {
    uint32_t image_index = this->swapchain->acquireNextImage(this->semaphore->semaphore);
    this->commands->submit(this->device->queue, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, image_index, { this->semaphore->semaphore });
    THROW_ERROR(vkQueueWaitIdle(this->device->queue));
    this->swapchain->present(this->device->queue, image_index);
  }

  std::shared_ptr<Device> device;
  std::unique_ptr<VulkanSemaphore> semaphore;
  std::unique_ptr<VulkanSwapchain> swapchain;
  std::unique_ptr<VulkanCommandBuffers> commands;
};


//*******************************************************************************************************************************
class FramebufferObject {
public:
  FramebufferObject(const std::shared_ptr<VulkanDevice> & device, VkCommandBuffer command, VkFormat color_format, VkExtent2D extent2d)
    : extent(extent2d)
  {
    this->clear_values = { { 0.0f, 0.0f, 0.0f, 0.0f },{ 1.0f, 0 } };
    VkExtent3D extent = { extent2d.width, extent2d.height, 1 };

    {
      VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
      VkComponentMapping component_mapping = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

      this->color_attachment = std::make_unique<ImageObject>(
        device,
        color_format,
        extent,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        subresource_range,
        component_mapping);
    }

    VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
    {
      VkImageSubresourceRange subresource_range = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
      VkComponentMapping component_mapping = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };

      this->depth_attachment = std::make_unique<ImageObject>(
        device,
        depth_format,
        extent,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        subresource_range,
        component_mapping);
    }

    std::vector<VkImageMemoryBarrier> memory_barriers = {
      this->color_attachment->setLayout(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
      this->depth_attachment->setLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    };

    vkCmdPipelineBarrier(
      command,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      0, 0, nullptr, 0, nullptr,
      static_cast<uint32_t>(memory_barriers.size()),
      memory_barriers.data());

    VkAttachmentDescription attachment_description;
    ::memset(&attachment_description, 0, sizeof(VkAttachmentDescription));
    attachment_description.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentDescription color_attachment_description = attachment_description;
    color_attachment_description.format = color_format;
    color_attachment_description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color_attachment_description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth_attachment_description = attachment_description;
    depth_attachment_description.format = depth_format;
    depth_attachment_description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depth_attachment_description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    std::vector<VkAttachmentDescription> attachments = {
      color_attachment_description, depth_attachment_description
    };
    std::vector<VkAttachmentReference> color_attachments = {
      { 0, color_attachment_description.initialLayout }
    };
    VkAttachmentReference depth_stencil_attachment = { 1, depth_attachment_description.finalLayout };

    VkSubpassDescription subpass_description;
    ::memset(&subpass_description, 0, sizeof(VkSubpassDescription));
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.colorAttachmentCount = static_cast<uint32_t>(color_attachments.size());
    subpass_description.pColorAttachments = color_attachments.data();
    subpass_description.pDepthStencilAttachment = &depth_stencil_attachment;

    std::vector<VkSubpassDescription> subpass_descriptions;
    subpass_descriptions.push_back(subpass_description);

    this->renderpass = std::make_unique<VulkanRenderPass>(device, attachments, subpass_descriptions);

    std::vector<VkImageView> framebuffer_attachments = { this->color_attachment->view->view, this->depth_attachment->view->view };
    this->framebuffer = std::make_unique<VulkanFramebuffer>(device, this->renderpass->render_pass, framebuffer_attachments, extent.width, extent.height, 1);
  }

  ~FramebufferObject() {}

  void begin(VkCommandBuffer cmd)
  {
    VkRect2D renderarea;
    renderarea.extent = this->extent;
    renderarea.offset = { 0, 0 };
    this->renderpass->begin(cmd, this->framebuffer->framebuffer, renderarea, this->clear_values, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
  }

  void end(VkCommandBuffer cmd)
  {
    this->renderpass->end(cmd);
  }

  std::unique_ptr<ImageObject> color_attachment;
  std::unique_ptr<ImageObject> depth_attachment;
  std::unique_ptr<VulkanRenderPass> renderpass;
  std::unique_ptr<VulkanFramebuffer> framebuffer;

  VkExtent2D extent;
  std::vector<VkClearValue> clear_values;
};

class FramebufferScope {
public:
  FramebufferScope(FramebufferObject * fbo, VkCommandBuffer command)
    : fbo(fbo), command(command)
  {
    this->fbo->begin(this->command);
  }
  ~FramebufferScope()
  {
    this->fbo->end(this->command);
  }
  VkCommandBuffer command;
  FramebufferObject * fbo;
};

//*******************************************************************************************************************************
class VulkanGraphicsPipeline {
public:
  VulkanGraphicsPipeline(const std::shared_ptr<VulkanDevice> & device, 
                         VkRenderPass render_pass,
                         VkPipelineCache pipeline_cache,
                         VkPipelineLayout pipeline_layout,
                         VkPrimitiveTopology primitive_topology,
                         VkPipelineRasterizationStateCreateInfo rasterization_state,
                         std::vector<VkDynamicState> dynamic_states,
                         std::vector<VkPipelineShaderStageCreateInfo> shaderstages,
                         std::vector<VkVertexInputBindingDescription> binding_descriptions,
                         std::vector<VkVertexInputAttributeDescription> attribute_descriptions)
    : device(device)
  {
    VkPipelineVertexInputStateCreateInfo vertex_input_state;
    ::memset(&vertex_input_state, 0, sizeof(VkPipelineVertexInputStateCreateInfo));
    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.vertexBindingDescriptionCount = (uint32_t)binding_descriptions.size();
    vertex_input_state.pVertexBindingDescriptions = binding_descriptions.data();
    vertex_input_state.vertexAttributeDescriptionCount = (uint32_t)attribute_descriptions.size();
    vertex_input_state.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
    ::memset(&input_assembly_state, 0, sizeof(VkPipelineInputAssemblyStateCreateInfo));
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.topology = primitive_topology;

    VkPipelineColorBlendAttachmentState blend_attachment_state;
    ::memset(&blend_attachment_state, 0, sizeof(VkPipelineColorBlendAttachmentState));
    blend_attachment_state.blendEnable = VK_TRUE;
    blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
    blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
    blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state;
    ::memset(&color_blend_state, 0, sizeof(VkPipelineColorBlendStateCreateInfo));
    color_blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state.attachmentCount = 1;
    color_blend_state.pAttachments = &blend_attachment_state;

    VkPipelineDynamicStateCreateInfo dynamic_state;
    ::memset(&dynamic_state, 0, sizeof(VkPipelineDynamicStateCreateInfo));
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pDynamicStates = dynamic_states.data();
    dynamic_state.dynamicStateCount = (uint32_t)dynamic_states.size();

    VkPipelineViewportStateCreateInfo viewport_state;
    ::memset(&viewport_state, 0, sizeof(VkPipelineViewportStateCreateInfo));
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
    ::memset(&depth_stencil_state, 0, sizeof(VkPipelineDepthStencilStateCreateInfo));
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.depthTestEnable = VK_TRUE;
    depth_stencil_state.depthWriteEnable = VK_TRUE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depth_stencil_state.back.failOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state.back.passOp = VK_STENCIL_OP_KEEP;
    depth_stencil_state.back.compareOp = VK_COMPARE_OP_ALWAYS;
    depth_stencil_state.front = depth_stencil_state.back;

    VkPipelineMultisampleStateCreateInfo multi_sample_state;
    ::memset(&multi_sample_state, 0, sizeof(VkPipelineMultisampleStateCreateInfo));
    multi_sample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multi_sample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkGraphicsPipelineCreateInfo create_info;
    ::memset(&create_info, 0, sizeof(VkGraphicsPipelineCreateInfo));
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.layout = pipeline_layout;
    create_info.pVertexInputState = &vertex_input_state;
    create_info.pInputAssemblyState = &input_assembly_state;
    create_info.pRasterizationState = &rasterization_state;
    create_info.pColorBlendState = &color_blend_state;
    create_info.pMultisampleState = &multi_sample_state;
    create_info.pViewportState = &viewport_state;
    create_info.pDepthStencilState = &depth_stencil_state;
    create_info.stageCount = (uint32_t)shaderstages.size();
    create_info.pStages = shaderstages.data();
    create_info.renderPass = render_pass;
    create_info.pDynamicState = &dynamic_state;

    THROW_ERROR(vkCreateGraphicsPipelines(this->device->device, pipeline_cache, 1, &create_info, nullptr, &this->pipeline));
  }


  ~VulkanGraphicsPipeline()
  {
    vkDestroyPipeline(this->device->device, this->pipeline, nullptr);
  }

  void bind(VkCommandBuffer buffer)
  {
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipeline);
  }

  VkPipeline pipeline;
  std::shared_ptr<VulkanDevice> device;
};


//*******************************************************************************************************************************

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

