#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <algorithm>
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
	} else if (result == VK_ERROR_OUT_OF_DATE_KHR) {                \
    throw VkErrorOutOfDateException();                            \
  }                                                               \
}                                                                 \

namespace {

  std::vector<const char*> string2char(const std::vector<std::string> & v) 
  {
    std::vector<const char*> result(v.size());
    std::transform(v.begin(), v.end(), result.begin(), [](const std::string & s) { return s.c_str(); });
    return result;
  }

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

  std::vector<std::string> set_difference(std::vector<std::string> required, std::vector<std::string> supported)
  {
    std::sort(required.begin(), required.end());
    std::sort(supported.begin(), supported.end());

    std::vector<std::string> difference;
    std::set_difference(required.begin(), required.end(), supported.begin(), supported.end(), std::back_inserter(difference));
    return difference;
  }
}

class VulkanCommandPool {
public:
  VulkanCommandPool(VkDevice device, VkCommandPoolCreateFlags flags, uint32_t queue_family_index)
    : device(device)
  {
    VkCommandPoolCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = flags;
    create_info.queueFamilyIndex = queue_family_index;

    THROW_ERROR(vkCreateCommandPool(this->device, &create_info, nullptr, &this->pool));
  }

  ~VulkanCommandPool()
  {
    vkDestroyCommandPool(this->device, this->pool, nullptr);
  }

  VkCommandPool pool;
  VkDevice device;
};

class VulkanPhysicalDevice {
public:
  VulkanPhysicalDevice(VkPhysicalDevice device)
    : device(device)
  {
    vkGetPhysicalDeviceFeatures(this->device, &this->features);
    vkGetPhysicalDeviceProperties(this->device, &this->properties);
    vkGetPhysicalDeviceMemoryProperties(this->device, &this->memory_properties);

    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(this->device, &count, nullptr);
    this->queue_family_properties.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(this->device, &count, this->queue_family_properties.data());

    THROW_ERROR(vkEnumerateDeviceLayerProperties(this->device, &count, nullptr));
    this->layer_properties.resize(count);
    THROW_ERROR(vkEnumerateDeviceLayerProperties(this->device, &count, this->layer_properties.data()));

    THROW_ERROR(vkEnumerateDeviceExtensionProperties(this->device, nullptr, &count, nullptr));
    this->extension_properties.resize(count);
    THROW_ERROR(vkEnumerateDeviceExtensionProperties(this->device, nullptr, &count, this->extension_properties.data()));
  }

  ~VulkanPhysicalDevice() {}

  uint32_t getQueueIndex(VkQueueFlags required_flags, std::vector<VkBool32> filter = std::vector<VkBool32>())
  {
    if (filter.empty()) {
      filter.resize(this->queue_family_properties.size());
      std::fill(filter.begin(), filter.end(), VK_TRUE);
    }
    if (filter.size() != this->queue_family_properties.size()) {
      throw std::runtime_error("VulkanDevice::getQueueIndex: invalid filter size");
    }
    // check for exact match of required flags
    for (uint32_t queue_index = 0; queue_index < this->queue_family_properties.size(); queue_index++) {
      if (this->queue_family_properties[queue_index].queueFlags == required_flags) {
        if (filter[queue_index]) {
          return queue_index;
        }
      }
    }
    // check for queue with all required flags set
    for (uint32_t queue_index = 0; queue_index < this->queue_family_properties.size(); queue_index++) {
      if ((this->queue_family_properties[queue_index].queueFlags & required_flags) == required_flags) {
        if (filter[queue_index]) {
          return queue_index;
        }
      }
    }
    throw std::runtime_error("VulkanDevice::getQueueIndex: could not find queue with required properties");
  }

  uint32_t getMemoryTypeIndex(VkMemoryRequirements requirements, VkMemoryPropertyFlags required_flags)
  {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
      if (((requirements.memoryTypeBits >> i) & 1) == 1) { // check for required memory type
        if ((this->memory_properties.memoryTypes[i].propertyFlags & required_flags) == required_flags) {
          return i;
        }
      }
    }
    throw std::runtime_error("VulkanDevice::getQueueIndex: could not find suitable memory type");
  }

  VkPhysicalDevice device;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceMemoryProperties memory_properties;
  std::vector<VkQueueFamilyProperties> queue_family_properties;
  std::vector<VkExtensionProperties> extension_properties;
  std::vector<VkLayerProperties> layer_properties;
};

class VulkanInstance {
public:
  VulkanInstance(const VkApplicationInfo & application_info,
                 const std::vector<std::string> & required_layers,
                 const std::vector<std::string> & required_extensions)
  {
    { // check that required layers are supported
      uint32_t count;
      THROW_ERROR(vkEnumerateInstanceLayerProperties(&count, nullptr));
      std::vector<VkLayerProperties> layer_properties(count);
      THROW_ERROR(vkEnumerateInstanceLayerProperties(&count, layer_properties.data()));

      if (!set_difference(required_layers, extract_layer_names(layer_properties)).empty()) {
        throw std::runtime_error("unsupported instance layers");
      }
    }
    { // check that required extensions are supported
      uint32_t count;
      THROW_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
      std::vector<VkExtensionProperties> extension_properties(count);
      THROW_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &count, extension_properties.data()));

      if (!set_difference(required_extensions, extract_extension_names(extension_properties)).empty()) {
        throw std::runtime_error("unsupported instance extensions");
      }
    }
    std::vector<const char*> layer_names = string2char(required_layers);
    std::vector<const char*> extension_names = string2char(required_extensions);

    VkInstanceCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.pApplicationInfo = &application_info;
    create_info.enabledLayerCount = static_cast<uint32_t>(layer_names.size());
    create_info.ppEnabledLayerNames = layer_names.data();
    create_info.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
    create_info.ppEnabledExtensionNames = extension_names.data();

    THROW_ERROR(vkCreateInstance(&create_info, nullptr, &this->instance));

    uint32_t physical_device_count;
    THROW_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    THROW_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()));

    for (const VkPhysicalDevice & physical_device : physical_devices) {
      this->physical_devices.push_back(VulkanPhysicalDevice(physical_device));
    }
    {
      vkQueuePresent = (PFN_vkQueuePresentKHR)vkGetInstanceProcAddr(this->instance, "vkQueuePresentKHR");
      vkCreateSwapchain = (PFN_vkCreateSwapchainKHR)vkGetInstanceProcAddr(this->instance, "vkCreateSwapchainKHR");
      vkAcquireNextImage = (PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(this->instance, "vkAcquireNextImageKHR");
      vkDestroySwapchain = (PFN_vkDestroySwapchainKHR)vkGetInstanceProcAddr(this->instance, "vkDestroySwapchainKHR");
      vkGetSwapchainImages = (PFN_vkGetSwapchainImagesKHR)vkGetInstanceProcAddr(this->instance, "vkGetSwapchainImagesKHR");
      vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(this->instance, "vkCreateDebugReportCallbackEXT");
      vkDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(this->instance, "vkDestroyDebugReportCallbackEXT");
      vkGetPhysicalDeviceSurfaceSupport = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(this->instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
      vkGetPhysicalDeviceSurfaceFormats = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)vkGetInstanceProcAddr(this->instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
      vkGetPhysicalDeviceSurfaceCapabilities = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)vkGetInstanceProcAddr(this->instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
      vkGetPhysicalDeviceSurfacePresentModes = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)vkGetInstanceProcAddr(this->instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

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
        throw std::runtime_error("VulkanFunctions::VulkanFunctions(): vkGetInstanceProcAddr failed.");
      }
    }
  }

  ~VulkanInstance()
  {
    vkDestroyInstance(this->instance, nullptr);
  }

  VulkanPhysicalDevice selectPhysicalDevice(const VkPhysicalDeviceFeatures & required_features) 
  {
    for (VulkanPhysicalDevice & physical_device : this->physical_devices) {
      if (physical_device.features.robustBufferAccess >= required_features.robustBufferAccess &&
          physical_device.features.fullDrawIndexUint32 >= required_features.fullDrawIndexUint32 &&
          physical_device.features.imageCubeArray >= required_features.imageCubeArray &&
          physical_device.features.independentBlend >= required_features.independentBlend &&
          physical_device.features.geometryShader >= required_features.geometryShader &&
          physical_device.features.tessellationShader >= required_features.tessellationShader &&
          physical_device.features.sampleRateShading >= required_features.sampleRateShading &&
          physical_device.features.dualSrcBlend >= required_features.dualSrcBlend &&
          physical_device.features.logicOp >= required_features.logicOp &&
          physical_device.features.multiDrawIndirect >= required_features.multiDrawIndirect &&
          physical_device.features.drawIndirectFirstInstance >= required_features.drawIndirectFirstInstance &&
          physical_device.features.depthClamp >= required_features.depthClamp &&
          physical_device.features.depthBiasClamp >= required_features.depthBiasClamp &&
          physical_device.features.fillModeNonSolid >= required_features.fillModeNonSolid &&
          physical_device.features.depthBounds >= required_features.depthBounds &&
          physical_device.features.wideLines >= required_features.wideLines &&
          physical_device.features.largePoints >= required_features.largePoints &&
          physical_device.features.alphaToOne >= required_features.alphaToOne &&
          physical_device.features.multiViewport >= required_features.multiViewport &&
          physical_device.features.samplerAnisotropy >= required_features.samplerAnisotropy &&
          physical_device.features.textureCompressionETC2 >= required_features.textureCompressionETC2 &&
          physical_device.features.textureCompressionASTC_LDR >= required_features.textureCompressionASTC_LDR &&
          physical_device.features.textureCompressionBC >= required_features.textureCompressionBC &&
          physical_device.features.occlusionQueryPrecise >= required_features.occlusionQueryPrecise &&
          physical_device.features.pipelineStatisticsQuery >= required_features.pipelineStatisticsQuery &&
          physical_device.features.vertexPipelineStoresAndAtomics >= required_features.vertexPipelineStoresAndAtomics &&
          physical_device.features.fragmentStoresAndAtomics >= required_features.fragmentStoresAndAtomics &&
          physical_device.features.shaderTessellationAndGeometryPointSize >= required_features.shaderTessellationAndGeometryPointSize &&
          physical_device.features.shaderImageGatherExtended >= required_features.shaderImageGatherExtended &&
          physical_device.features.shaderStorageImageExtendedFormats >= required_features.shaderStorageImageExtendedFormats &&
          physical_device.features.shaderStorageImageMultisample >= required_features.shaderStorageImageMultisample &&
          physical_device.features.shaderStorageImageReadWithoutFormat >= required_features.shaderStorageImageReadWithoutFormat &&
          physical_device.features.shaderStorageImageWriteWithoutFormat >= required_features.shaderStorageImageWriteWithoutFormat &&
          physical_device.features.shaderUniformBufferArrayDynamicIndexing >= required_features.shaderUniformBufferArrayDynamicIndexing &&
          physical_device.features.shaderSampledImageArrayDynamicIndexing >= required_features.shaderSampledImageArrayDynamicIndexing &&
          physical_device.features.shaderStorageBufferArrayDynamicIndexing >= required_features.shaderStorageBufferArrayDynamicIndexing &&
          physical_device.features.shaderStorageImageArrayDynamicIndexing >= required_features.shaderStorageImageArrayDynamicIndexing &&
          physical_device.features.shaderClipDistance >= required_features.shaderClipDistance &&
          physical_device.features.shaderCullDistance >= required_features.shaderCullDistance &&
          physical_device.features.shaderFloat64 >= required_features.shaderFloat64 &&
          physical_device.features.shaderInt64 >= required_features.shaderInt64 &&
          physical_device.features.shaderInt16 >= required_features.shaderInt16 &&
          physical_device.features.shaderResourceResidency >= required_features.shaderResourceResidency &&
          physical_device.features.shaderResourceMinLod >= required_features.shaderResourceMinLod &&
          physical_device.features.sparseBinding >= required_features.sparseBinding &&
          physical_device.features.sparseResidencyBuffer >= required_features.sparseResidencyBuffer &&
          physical_device.features.sparseResidencyImage2D >= required_features.sparseResidencyImage2D &&
          physical_device.features.sparseResidencyImage3D >= required_features.sparseResidencyImage3D &&
          physical_device.features.sparseResidency2Samples >= required_features.sparseResidency2Samples &&
          physical_device.features.sparseResidency4Samples >= required_features.sparseResidency4Samples &&
          physical_device.features.sparseResidency8Samples >= required_features.sparseResidency8Samples &&
          physical_device.features.sparseResidency16Samples >= required_features.sparseResidency16Samples &&
          physical_device.features.sparseResidencyAliased >= required_features.sparseResidencyAliased &&
          physical_device.features.variableMultisampleRate >= required_features.variableMultisampleRate &&
          physical_device.features.inheritedQueries >= required_features.inheritedQueries) 
      {
        return physical_device;
      }
    }
    throw std::runtime_error("Could not find physical device with the required features");
  }

  VkInstance instance;

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

  std::vector<VulkanPhysicalDevice> physical_devices;
};

class VulkanDevice {
public:
  VulkanDevice(const VulkanPhysicalDevice & physical_device,
               const VkPhysicalDeviceFeatures & enabled_features,
               const std::vector<std::string> & required_layers,
               const std::vector<std::string> & required_extensions,
               const std::vector<VkDeviceQueueCreateInfo> & queue_create_infos)
    : physical_device(physical_device)
  {
    if (!set_difference(required_layers, extract_layer_names(physical_device.layer_properties)).empty()) {
      throw std::runtime_error("unsupported device layers");
    }
    if (!set_difference(required_extensions, extract_extension_names(physical_device.extension_properties)).empty()) {
      throw std::runtime_error("unsupported device extensions");
    }
    std::vector<const char*> layer_names = string2char(required_layers);
    std::vector<const char*> extension_names = string2char(required_extensions);

    VkDeviceCreateInfo device_create_info;
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = nullptr;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.enabledLayerCount = static_cast<uint32_t>(layer_names.size());
    device_create_info.ppEnabledLayerNames = layer_names.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(extension_names.size());
    device_create_info.ppEnabledExtensionNames = extension_names.data();
    device_create_info.pEnabledFeatures = &enabled_features;

    THROW_ERROR(vkCreateDevice(this->physical_device.device, &device_create_info, nullptr, &this->device));

    this->default_queue_index = queue_create_infos[0].queueFamilyIndex;
    vkGetDeviceQueue(this->device, this->default_queue_index, 0, &this->default_queue);

    VkCommandPoolCreateInfo pool_create_info;
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.pNext = nullptr;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = queue_create_infos[0].queueFamilyIndex;

    THROW_ERROR(vkCreateCommandPool(this->device, &pool_create_info, nullptr, &this->default_pool));
  }

  ~VulkanDevice()
  {
    vkDestroyCommandPool(this->device, this->default_pool, nullptr);
    vkDestroyDevice(this->device, nullptr);
  }

  VkDevice device;
  uint32_t default_queue_index;
  VkQueue default_queue;
  VkQueue compute_queue;
  VkQueue transfer_queue;
  VkCommandPool default_pool;
  VkCommandPool compute_pool;
  VkCommandPool transfer_pool;
  VulkanPhysicalDevice physical_device;
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

class VulkanSwapchain {
public:
  VulkanSwapchain(const std::shared_ptr<VulkanDevice> & device, 
                  const std::shared_ptr<VulkanInstance> & vulkan,
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
    create_info.presentMode = presentMode;
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
    uint32_t count;
    THROW_ERROR(vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &count, nullptr));
    std::vector<VkImage> images(count);
    THROW_ERROR(vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &count, images.data()));
    return images;
  }

  uint32_t getNextImageIndex(const std::shared_ptr<VulkanSemaphore> & semaphore)
  {
    uint32_t index;
    THROW_ERROR(this->vulkan->vkAcquireNextImage(this->device->device, this->swapchain, UINT64_MAX, semaphore->semaphore, nullptr, &index));
    return index;
  };


  VkSwapchainKHR swapchain;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanInstance> vulkan;
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
                       uint32_t count,
                       VkDescriptorSetLayout descriptor_set_layout)
    : device(device), pool(pool)
  {
    VkDescriptorSetAllocateInfo allocate_info;
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.descriptorPool = this->pool->pool;
    allocate_info.descriptorSetCount = count;
    allocate_info.pSetLayouts = &descriptor_set_layout;

    this->descriptor_sets.resize(count);
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

class VulkanFence {
public:
  VulkanFence(const std::shared_ptr<VulkanDevice> & device)
    : device(device)
  {
    VkFenceCreateInfo create_info = {
      VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, // sType
      nullptr,                             // pNext
      VK_FENCE_CREATE_SIGNALED_BIT         // flags
    };

    THROW_ERROR(vkCreateFence(this->device->device, &create_info, nullptr, &this->fence));
  }
  
  ~VulkanFence() 
  {
    vkDestroyFence(this->device->device, this->fence, nullptr);
  }

  VkFence fence;
  std::shared_ptr<VulkanDevice> device;
};

class VulkanCommandBuffers {
public:
  VulkanCommandBuffers(const std::shared_ptr<VulkanDevice> & device, 
                       size_t count = 1,
                       VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    : device(device)
  {
    VkCommandBufferAllocateInfo allocate_info;
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.pNext = nullptr;
    allocate_info.commandPool = device->default_pool;
    allocate_info.level = level;
    allocate_info.commandBufferCount = static_cast<uint32_t>(count);

    this->buffers.resize(count);
    THROW_ERROR(vkAllocateCommandBuffers(this->device->device, &allocate_info, this->buffers.data()));
  }

  ~VulkanCommandBuffers()
  {
    vkFreeCommandBuffers(this->device->device, this->device->default_pool, static_cast<uint32_t>(this->buffers.size()), this->buffers.data());
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

    vkGetImageMemoryRequirements(this->device->device, this->image, &this->memory_requirements);
  }

  ~VulkanImage()
  {
    vkDestroyImage(this->device->device, this->image, nullptr);
  }

  VkImage image;
  VkMemoryRequirements memory_requirements;
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
    vkGetBufferMemoryRequirements(this->device->device, this->buffer, &this->memory_requirements);
  }

  ~VulkanBuffer()
  {
    vkDestroyBuffer(this->device->device, this->buffer, nullptr);
  }

  VkBuffer buffer;
  VkMemoryRequirements memory_requirements;
  std::shared_ptr<VulkanDevice> device;
};

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
    allocate_info.memoryTypeIndex = device->physical_device.getMemoryTypeIndex(requirements, flags);

    THROW_ERROR(vkAllocateMemory(this->device->device, &allocate_info, &allocation_callbacks, &this->memory));
  }

  VulkanMemory(const std::shared_ptr<VulkanDevice> & device, const std::unique_ptr<VulkanImage> & image, VkMemoryPropertyFlags flags)
    : VulkanMemory(device, image->memory_requirements, flags)
  {
    THROW_ERROR(vkBindImageMemory(this->device->device, image->image, this->memory, 0));
  }

  VulkanMemory(const std::shared_ptr<VulkanDevice> & device, const std::unique_ptr<VulkanBuffer> & buffer, VkMemoryPropertyFlags flags)
    : VulkanMemory(device, buffer->memory_requirements, flags)
  {
    THROW_ERROR(vkBindBufferMemory(this->device->device, buffer->buffer, this->memory, 0));
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


class VulkanRenderpass {
public:
  VulkanRenderpass(const std::shared_ptr<VulkanDevice> & device,
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

    THROW_ERROR(vkCreateRenderPass(this->device->device, &create_info, nullptr, &this->renderpass));
  }

  ~VulkanRenderpass()
  {
    vkDestroyRenderPass(this->device->device, this->renderpass, nullptr);
  }

  VkRenderPass renderpass;
  std::shared_ptr<VulkanDevice> device;
};


class VulkanFramebuffer {
public:
  VulkanFramebuffer(const std::shared_ptr<VulkanDevice> & device,
                    const std::shared_ptr<VulkanRenderpass> & renderpass,
                    std::vector<VkImageView> attachments, 
                    VkExtent2D extent,
                    uint32_t layers)
    : device(device)
  {
    VkFramebufferCreateInfo create_info;
    create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0;
    create_info.renderPass = renderpass->renderpass;
    create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    create_info.pAttachments = attachments.data();
    create_info.width = extent.width;
    create_info.height = extent.height;
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
