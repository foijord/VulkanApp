#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace {
  class VkTimeoutException : public std::exception {};
  class VkNotReadyException : public std::exception {};
  class VkIncompleteException : public std::exception {};
  class VkSuboptimalException : public std::exception {};
  class VkErrorOutOfDateException : public std::exception {};
  class VkErrorDeviceLostException : public std::exception {};
  class VkErrorSurfaceLostException : public std::exception {};
  class VkErrorInvalidShaderException : public std::exception {};
  class VkErrorFragmentedPoolException : public std::exception {};
  class VkErrorTooManyObjectsException : public std::exception {};
  class VkErrorLayerNotPresentException : public std::exception {};
  class VkErrorMemoryMapFailedException : public std::exception {};
  class VkErrorOutOfHostMemoryException : public std::exception {};
  class VkErrorOutOfPoolMemoryException : public std::exception {};
  class VkErrorValidationFailedException : public std::exception {};
  class VkErrorNativeWindowInUseException : public std::exception {};
  class VkErrorFeatureNotPresentException : public std::exception {};
  class VkErrorOutOfDeviceMemoryException : public std::exception {};
  class VkErrorFormatNotSupportedException : public std::exception {};
  class VkErrorIncompatibleDriverException : public std::exception {};
  class VkErrorExtensionNotPresentException : public std::exception {};
  class VkErrorIncompatibleDisplayException : public std::exception {};
  class VkErrorInitializationFailedException : public std::exception {};
  class VkErrorInvalidExternalHandleException : public std::exception {};

#define THROW_ON_ERROR(__function__) {                                                           \
	VkResult __result__ = (__function__);                                                       \
  switch (__result__) {                                                                       \
    case VK_TIMEOUT: throw VkTimeoutException();                                              \
    case VK_NOT_READY: throw VkNotReadyException();                                           \
    case VK_INCOMPLETE: throw VkIncompleteException();                                        \
    case VK_SUBOPTIMAL_KHR: throw VkSuboptimalException();                                    \
    case VK_ERROR_DEVICE_LOST: throw VkErrorDeviceLostException();                            \
    case VK_ERROR_OUT_OF_DATE_KHR: throw VkErrorOutOfDateException();                         \
    case VK_ERROR_SURFACE_LOST_KHR: throw VkErrorSurfaceLostException();                      \
    case VK_ERROR_FRAGMENTED_POOL: throw VkErrorFragmentedPoolException();                    \
    case VK_ERROR_INVALID_SHADER_NV: throw VkErrorInvalidShaderException();                   \
    case VK_ERROR_TOO_MANY_OBJECTS: throw VkErrorTooManyObjectsException();                   \
    case VK_ERROR_MEMORY_MAP_FAILED: throw VkErrorMemoryMapFailedException();                 \
    case VK_ERROR_LAYER_NOT_PRESENT: throw VkErrorLayerNotPresentException();                 \
    case VK_ERROR_OUT_OF_HOST_MEMORY: throw VkErrorOutOfHostMemoryException();                \
    case VK_ERROR_FEATURE_NOT_PRESENT: throw VkErrorFeatureNotPresentException();             \
    case VK_ERROR_INCOMPATIBLE_DRIVER: throw VkErrorIncompatibleDriverException();            \
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: throw VkErrorOutOfDeviceMemoryException();            \
    case VK_ERROR_VALIDATION_FAILED_EXT: throw VkErrorValidationFailedException();            \
    case VK_ERROR_OUT_OF_POOL_MEMORY_KHR: throw VkErrorOutOfPoolMemoryException();            \
    case VK_ERROR_FORMAT_NOT_SUPPORTED: throw VkErrorFormatNotSupportedException();           \
    case VK_ERROR_EXTENSION_NOT_PRESENT: throw VkErrorExtensionNotPresentException();         \
    case VK_ERROR_INITIALIZATION_FAILED: throw VkErrorInitializationFailedException();        \
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: throw VkErrorNativeWindowInUseException();        \
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: throw VkErrorIncompatibleDisplayException();      \
    case VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR: throw VkErrorInvalidExternalHandleException(); \
  }                                                                                           \
}                                                                                             \

}

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

    THROW_ON_ERROR(vkEnumerateDeviceLayerProperties(this->device, &count, nullptr));
    this->layer_properties.resize(count);
    THROW_ON_ERROR(vkEnumerateDeviceLayerProperties(this->device, &count, this->layer_properties.data()));

    THROW_ON_ERROR(vkEnumerateDeviceExtensionProperties(this->device, nullptr, &count, nullptr));
    this->extension_properties.resize(count);
    THROW_ON_ERROR(vkEnumerateDeviceExtensionProperties(this->device, nullptr, &count, this->extension_properties.data()));
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
                 const std::vector<const char *> & required_layers,
                 const std::vector<const char *> & required_extensions)
  {
    uint32_t count;
    // check that required layers are supported
    THROW_ON_ERROR(vkEnumerateInstanceLayerProperties(&count, nullptr));
    std::vector<VkLayerProperties> layer_properties(count);
    THROW_ON_ERROR(vkEnumerateInstanceLayerProperties(&count, layer_properties.data()));

    THROW_ON_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));
    std::vector<VkExtensionProperties> extension_properties(count);
    THROW_ON_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &count, extension_properties.data()));

    std::for_each(required_layers.begin(), required_layers.end(), 
      [&](const char * layer_name) {
        for (VkLayerProperties properties : layer_properties)
          if (std::strcmp(layer_name, properties.layerName) == 0)
            return;
        throw std::runtime_error("unsupported layer: " + std::string(layer_name));
      });

    std::for_each(required_extensions.begin(), required_extensions.end(), 
      [&](const char * extension_name) {
        for (VkExtensionProperties properties : extension_properties)
          if (std::strcmp(extension_name, properties.extensionName) == 0)
            return;
      throw std::runtime_error("unsupported extension: " + std::string(extension_name));
    });

    VkInstanceCreateInfo create_info{
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,            // sType 
      nullptr,                                           // pNext 
      0,                                                 // flags
      &application_info,                                 // pApplicationInfo
      static_cast<uint32_t>(required_layers.size()),     // enabledLayerCount
      required_layers.data(),                            // ppEnabledLayerNames
      static_cast<uint32_t>(required_extensions.size()), // enabledExtensionCount
      required_extensions.data()                         // ppEnabledExtensionNames
    };
    THROW_ON_ERROR(vkCreateInstance(&create_info, nullptr, &this->instance));

    uint32_t physical_device_count;
    THROW_ON_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    THROW_ON_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()));

    for (const VkPhysicalDevice & physical_device : physical_devices) {
      this->physical_devices.push_back(VulkanPhysicalDevice(physical_device));
    }
    {
      auto get_proc_address = [](VkInstance instance, const std::string & name) {
        PFN_vkVoidFunction address = vkGetInstanceProcAddr(instance, name.c_str());
        if (!address) {
          throw std::runtime_error("vkGetInstanceProcAddr failed for " + name);
        }
        return address;
      };

      this->vkQueuePresent = (PFN_vkQueuePresentKHR)get_proc_address(this->instance, "vkQueuePresentKHR");
      this->vkCreateSwapchain = (PFN_vkCreateSwapchainKHR)get_proc_address(this->instance, "vkCreateSwapchainKHR");
      this->vkAcquireNextImage = (PFN_vkAcquireNextImageKHR)get_proc_address(this->instance, "vkAcquireNextImageKHR");
      this->vkDestroySwapchain = (PFN_vkDestroySwapchainKHR)get_proc_address(this->instance, "vkDestroySwapchainKHR");
      this->vkGetSwapchainImages = (PFN_vkGetSwapchainImagesKHR)get_proc_address(this->instance, "vkGetSwapchainImagesKHR");
      this->vkCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)get_proc_address(this->instance, "vkCreateDebugReportCallbackEXT");
      this->vkDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)get_proc_address(this->instance, "vkDestroyDebugReportCallbackEXT");
      this->vkGetPhysicalDeviceSurfaceSupport = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)get_proc_address(this->instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
      this->vkGetPhysicalDeviceSurfaceFormats = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)get_proc_address(this->instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
      this->vkGetPhysicalDeviceSurfaceCapabilities = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)get_proc_address(this->instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
      this->vkGetPhysicalDeviceSurfacePresentModes = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)get_proc_address(this->instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    }
  }

  ~VulkanInstance()
  {
    vkDestroyInstance(this->instance, nullptr);
  }

  VulkanPhysicalDevice selectPhysicalDevice(const VkPhysicalDeviceFeatures & required_features) 
  {
    for (VulkanPhysicalDevice & physical_device : this->physical_devices) {
      if (physical_device.features.logicOp >= required_features.logicOp &&
          physical_device.features.wideLines >= required_features.wideLines &&
          physical_device.features.depthClamp >= required_features.depthClamp &&
          physical_device.features.alphaToOne >= required_features.alphaToOne &&
          physical_device.features.depthBounds >= required_features.depthBounds &&
          physical_device.features.largePoints >= required_features.largePoints &&
          physical_device.features.shaderInt64 >= required_features.shaderInt64 &&
          physical_device.features.shaderInt16 >= required_features.shaderInt16 &&
          physical_device.features.dualSrcBlend >= required_features.dualSrcBlend &&
          physical_device.features.multiViewport >= required_features.multiViewport &&
          physical_device.features.shaderFloat64 >= required_features.shaderFloat64 &&
          physical_device.features.sparseBinding >= required_features.sparseBinding &&
          physical_device.features.imageCubeArray >= required_features.imageCubeArray &&
          physical_device.features.geometryShader >= required_features.geometryShader &&
          physical_device.features.depthBiasClamp >= required_features.depthBiasClamp &&
          physical_device.features.independentBlend >= required_features.independentBlend &&
          physical_device.features.fillModeNonSolid >= required_features.fillModeNonSolid &&
          physical_device.features.inheritedQueries >= required_features.inheritedQueries &&
          physical_device.features.sampleRateShading >= required_features.sampleRateShading &&
          physical_device.features.multiDrawIndirect >= required_features.multiDrawIndirect &&
          physical_device.features.samplerAnisotropy >= required_features.samplerAnisotropy &&
          physical_device.features.robustBufferAccess >= required_features.robustBufferAccess &&
          physical_device.features.tessellationShader >= required_features.tessellationShader &&
          physical_device.features.shaderClipDistance >= required_features.shaderClipDistance &&
          physical_device.features.shaderCullDistance >= required_features.shaderCullDistance &&
          physical_device.features.fullDrawIndexUint32 >= required_features.fullDrawIndexUint32 &&
          physical_device.features.textureCompressionBC >= required_features.textureCompressionBC &&
          physical_device.features.shaderResourceMinLod >= required_features.shaderResourceMinLod &&
          physical_device.features.occlusionQueryPrecise >= required_features.occlusionQueryPrecise &&
          physical_device.features.sparseResidencyBuffer >= required_features.sparseResidencyBuffer &&
          physical_device.features.textureCompressionETC2 >= required_features.textureCompressionETC2 &&
          physical_device.features.sparseResidencyImage2D >= required_features.sparseResidencyImage2D &&
          physical_device.features.sparseResidencyImage3D >= required_features.sparseResidencyImage3D &&
          physical_device.features.sparseResidencyAliased >= required_features.sparseResidencyAliased &&
          physical_device.features.pipelineStatisticsQuery >= required_features.pipelineStatisticsQuery &&
          physical_device.features.shaderResourceResidency >= required_features.shaderResourceResidency &&
          physical_device.features.sparseResidency2Samples >= required_features.sparseResidency2Samples &&
          physical_device.features.sparseResidency4Samples >= required_features.sparseResidency4Samples &&
          physical_device.features.sparseResidency8Samples >= required_features.sparseResidency8Samples &&
          physical_device.features.variableMultisampleRate >= required_features.variableMultisampleRate &&
          physical_device.features.sparseResidency16Samples >= required_features.sparseResidency16Samples &&
          physical_device.features.fragmentStoresAndAtomics >= required_features.fragmentStoresAndAtomics &&
          physical_device.features.drawIndirectFirstInstance >= required_features.drawIndirectFirstInstance &&
          physical_device.features.shaderImageGatherExtended >= required_features.shaderImageGatherExtended &&
          physical_device.features.textureCompressionASTC_LDR >= required_features.textureCompressionASTC_LDR &&
          physical_device.features.shaderStorageImageMultisample >= required_features.shaderStorageImageMultisample &&
          physical_device.features.vertexPipelineStoresAndAtomics >= required_features.vertexPipelineStoresAndAtomics &&
          physical_device.features.shaderStorageImageExtendedFormats >= required_features.shaderStorageImageExtendedFormats &&
          physical_device.features.shaderStorageImageReadWithoutFormat >= required_features.shaderStorageImageReadWithoutFormat &&
          physical_device.features.shaderStorageImageWriteWithoutFormat >= required_features.shaderStorageImageWriteWithoutFormat &&
          physical_device.features.shaderTessellationAndGeometryPointSize >= required_features.shaderTessellationAndGeometryPointSize &&
          physical_device.features.shaderSampledImageArrayDynamicIndexing >= required_features.shaderSampledImageArrayDynamicIndexing &&
          physical_device.features.shaderStorageImageArrayDynamicIndexing >= required_features.shaderStorageImageArrayDynamicIndexing &&
          physical_device.features.shaderUniformBufferArrayDynamicIndexing >= required_features.shaderUniformBufferArrayDynamicIndexing &&
          physical_device.features.shaderStorageBufferArrayDynamicIndexing >= required_features.shaderStorageBufferArrayDynamicIndexing) 
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

class VulkanLogicalDevice {
public:
  VulkanLogicalDevice(const VulkanPhysicalDevice & physical_device,
                      const VkPhysicalDeviceFeatures & enabled_features,
                      const std::vector<const char *> & required_layers,
                      const std::vector<const char *> & required_extensions,
                      const std::vector<VkDeviceQueueCreateInfo> & queue_create_infos)
    : physical_device(physical_device)
  {
    std::for_each(required_layers.begin(), required_layers.end(),
      [&](const char * layer_name) {
      for (VkLayerProperties properties : physical_device.layer_properties)
        if (std::strcmp(layer_name, properties.layerName) == 0)
          return;
      throw std::runtime_error("unsupported layer: " + std::string(layer_name));
    });

    std::for_each(required_extensions.begin(), required_extensions.end(),
      [&](const char * extension_name) {
      for (VkExtensionProperties properties : physical_device.extension_properties)
        if (std::strcmp(extension_name, properties.extensionName) == 0)
          return;
      throw std::runtime_error("unsupported extension: " + std::string(extension_name));
    });

    VkDeviceCreateInfo device_create_info {
      VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,                 // sType
      nullptr,                                              // pNext
      0,                                                    // flags
      static_cast<uint32_t>(queue_create_infos.size()),     // queueCreateInfoCount
      queue_create_infos.data(),                            // pQueueCreateInfos
      static_cast<uint32_t>(required_layers.size()),        // enabledLayerCount
      required_layers.data(),                               // ppEnabledLayerNames
      static_cast<uint32_t>(required_extensions.size()),    // enabledExtensionCount
      required_extensions.data(),                           // ppEnabledExtensionNames
      &enabled_features,                                    // pEnabledFeatures
    };
    THROW_ON_ERROR(vkCreateDevice(this->physical_device.device, &device_create_info, nullptr, &this->device));

    this->default_queue_index = queue_create_infos[0].queueFamilyIndex;
    vkGetDeviceQueue(this->device, this->default_queue_index, 0, &this->default_queue);
  }

  ~VulkanLogicalDevice()
  {
    vkDestroyDevice(this->device, nullptr);
  }

  VkDevice device;
  VkQueue default_queue;
  uint32_t default_queue_index;
  VulkanPhysicalDevice physical_device;
};

class VulkanCommandPool {
public:
  VulkanCommandPool(VkDevice device, 
                    VkCommandPoolCreateFlags flags, 
                    uint32_t queue_family_index)
    : device(device)
  {
    VkCommandPoolCreateInfo create_info {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // sType
      nullptr,                                    // pNext 
      flags,                                      // flags
      queue_family_index                          // queueFamilyIndex 
    };
    THROW_ON_ERROR(vkCreateCommandPool(this->device, &create_info, nullptr, &this->pool));
  }

  ~VulkanCommandPool()
  {
    vkDestroyCommandPool(this->device, this->pool, nullptr);
  }

  VkDevice device;
  VkCommandPool pool;
};

class VulkanDevice {
public:
  VulkanDevice(const VulkanPhysicalDevice & physical_device,
               const VkPhysicalDeviceFeatures & enabled_features,
               const std::vector<const char *> & required_layers,
               const std::vector<const char *> & required_extensions,
               const std::vector<VkDeviceQueueCreateInfo> & queue_create_infos)
    : physical_device(physical_device)
  {
    this->logical_device = std::make_unique<VulkanLogicalDevice>(physical_device, 
                                                                 enabled_features, 
                                                                 required_layers, 
                                                                 required_extensions, 
                                                                 queue_create_infos);

    this->device = this->logical_device->device;
    this->default_queue = this->logical_device->default_queue;
    this->default_queue_index = this->logical_device->default_queue_index;

    this->pool = std::make_unique<VulkanCommandPool>(this->device, 
                                                     VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, 
                                                     this->default_queue_index);
    this->default_pool = this->pool->pool;
  }

  ~VulkanDevice() {}

  VkDevice device;
  VkQueue default_queue;
  uint32_t default_queue_index;
  VkCommandPool default_pool;
  VulkanPhysicalDevice physical_device;
  
private:
  std::unique_ptr<VulkanLogicalDevice> logical_device;
  std::unique_ptr<VulkanCommandPool> pool;
};

class VulkanDebugCallback {
public:
  VulkanDebugCallback(const std::shared_ptr<VulkanInstance> & vulkan,
                      VkDebugReportFlagsEXT flags,
                      PFN_vkDebugReportCallbackEXT callback,
                      void * pUserData = nullptr)
    : vulkan(vulkan)
  {
    VkDebugReportCallbackCreateInfoEXT create_info {
      VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT, // sType
      nullptr,                                        // pNext  
      flags,                                          // flags  
      callback,                                       // pfnCallback  
      nullptr,                                        // pUserData 
    };

    THROW_ON_ERROR(this->vulkan->vkCreateDebugReportCallback(this->vulkan->instance, &create_info, nullptr, &this->callback));
  }

  ~VulkanDebugCallback()
  {
    this->vulkan->vkDestroyDebugReportCallback(this->vulkan->instance, this->callback, nullptr);
  }

  VkDebugReportCallbackEXT callback;
  std::shared_ptr<VulkanInstance> vulkan;
};

class VulkanSemaphore {
public:
  VulkanSemaphore(const std::shared_ptr<VulkanDevice> & device)
    : device(device)
  {
    VkSemaphoreCreateInfo create_info {
      VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, // sType
      nullptr,                                 // pNext
      0                                        // flags (reserved for future use)
    };
    THROW_ON_ERROR(vkCreateSemaphore(this->device->device, &create_info, nullptr, &this->semaphore));
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
    VkSwapchainCreateInfoKHR create_info {
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, // sType
      nullptr,                                     // pNext
      0,                                           // flags (reserved for future use)
      surface,
      minImageCount,
      imageFormat,
      imageColorSpace,
      imageExtent,
      imageArrayLayers,
      imageUsage,
      imageSharingMode,
      static_cast<uint32_t>(queueFamilyIndices.size()),
      queueFamilyIndices.data(),
      preTransform,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,           // compositeAlpha 
      presentMode,
      clipped,
      oldSwapchain
    };

    THROW_ON_ERROR(this->vulkan->vkCreateSwapchain(this->device->device, &create_info, nullptr, &this->swapchain));
  }

  ~VulkanSwapchain()
  {
    this->vulkan->vkDestroySwapchain(this->device->device, this->swapchain, nullptr);
  }

  std::vector<VkImage> getSwapchainImages()
  {
    uint32_t count;
    THROW_ON_ERROR(vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &count, nullptr));
    std::vector<VkImage> images(count);
    THROW_ON_ERROR(vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &count, images.data()));
    return images;
  }

  uint32_t getNextImageIndex(const std::shared_ptr<VulkanSemaphore> & semaphore)
  {
    uint32_t index;
    THROW_ON_ERROR(this->vulkan->vkAcquireNextImage(this->device->device, this->swapchain, UINT64_MAX, semaphore->semaphore, nullptr, &index));
    return index;
  };


  VkSwapchainKHR swapchain;
  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanInstance> vulkan;
};

class VulkanDescriptorPool {
public:
  VulkanDescriptorPool(const std::shared_ptr<VulkanDevice> & device, 
                       std::vector<VkDescriptorPoolSize> descriptor_pool_sizes)
    : device(device)
  {
    VkDescriptorPoolCreateInfo create_info {
    VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,         // sType 
      nullptr,                                             // pNext
      VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,   // flags
      1,                                                   // maxSets
      static_cast<uint32_t>(descriptor_pool_sizes.size()), // poolSizeCount
      descriptor_pool_sizes.data()                         // pPoolSizes
    };

    THROW_ON_ERROR(vkCreateDescriptorPool(this->device->device, &create_info, nullptr, &this->pool));
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
  VulkanDescriptorSetLayout(const std::shared_ptr<VulkanDevice> & device, 
                            std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings)
    : device(device)
  {
    VkDescriptorSetLayoutCreateInfo create_info {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,          // sType
      nullptr,                                                      // pNext
      0,                                                            // flags
      static_cast<uint32_t>(descriptor_set_layout_bindings.size()), // bindingCount
      descriptor_set_layout_bindings.data()                         // pBindings
    };

    THROW_ON_ERROR(vkCreateDescriptorSetLayout(this->device->device, &create_info, nullptr, &this->layout));
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
    VkDescriptorSetAllocateInfo allocate_info {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, // sType
      nullptr,                                        // pNext
      this->pool->pool,                               // descriptorPool
      count,                                          // descriptorSetCount
      &descriptor_set_layout                          // pSetLayouts
    };

    this->descriptor_sets.resize(count);
    THROW_ON_ERROR(vkAllocateDescriptorSets(this->device->device, &allocate_info, this->descriptor_sets.data()));
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

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanDescriptorPool> pool;
  std::vector<VkDescriptorSet> descriptor_sets;
};

class VulkanFence {
public:
  VulkanFence(const std::shared_ptr<VulkanDevice> & device)
    : device(device)
  {
    VkFenceCreateInfo create_info {
      VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, // sType
      nullptr,                             // pNext
      VK_FENCE_CREATE_SIGNALED_BIT         // flags
    };

    THROW_ON_ERROR(vkCreateFence(this->device->device, &create_info, nullptr, &this->fence));
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
    VkCommandBufferAllocateInfo allocate_info {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType 
      nullptr,                                        // pNext 
      device->default_pool,                           // commandPool 
      level,                                          // level 
      static_cast<uint32_t>(count),                   // commandBufferCount 
    };

    this->buffers.resize(count);
    THROW_ON_ERROR(vkAllocateCommandBuffers(this->device->device, &allocate_info, this->buffers.data()));
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
    VkCommandBufferInheritanceInfo inheritance_info {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, // sType
      nullptr,                                           // pNext
      renderpass,                                        // renderPass 
      subpass,                                           // subpass 
      framebuffer,                                       // framebuffer
      VK_FALSE,                                          // occlusionQueryEnable
      0,                                                 // queryFlags 
      0,                                                 // pipelineStatistics 
    };

    VkCommandBufferBeginInfo begin_info {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType 
      nullptr,                                     // pNext 
      flags,                                       // flags 
      &inheritance_info,                           // pInheritanceInfo 
    };

    THROW_ON_ERROR(vkBeginCommandBuffer(this->buffers[buffer_index], &begin_info));
  }

  void end(size_t buffer_index = 0)
  {
    THROW_ON_ERROR(vkEndCommandBuffer(this->buffers[buffer_index]));
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
    VkSubmitInfo submit_info {
      VK_STRUCTURE_TYPE_SUBMIT_INFO,                   // sType 
      nullptr,                                         // pNext  
      static_cast<uint32_t>(wait_semaphores.size()),   // waitSemaphoreCount  
      wait_semaphores.data(),                          // pWaitSemaphores  
      &flags,                                          // pWaitDstStageMask  
      static_cast<uint32_t>(buffers.size()),           // commandBufferCount  
      buffers.data(),                                  // pCommandBuffers 
      static_cast<uint32_t>(signal_semaphores.size()), // signalSemaphoreCount
      signal_semaphores.data(),                        // pSignalSemaphores
    };
    THROW_ON_ERROR(vkQueueSubmit(queue, 1, &submit_info, fence));
  }

  VkCommandBuffer buffer(size_t buffer_index = 0)
  {
    return this->buffers[buffer_index];
  }

  std::shared_ptr<VulkanDevice> device;
  std::vector<VkCommandBuffer> buffers;
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
    VkImageCreateInfo create_info {
      VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,                // sType  
      nullptr,                                            // pNext  
      flags,                                              // flags  
      image_type,                                         // imageType  
      format,                                             // format  
      extent,                                             // extent  
      mip_levels,                                         // mipLevels  
      array_layers,                                       // arrayLayers  
      samples,                                            // samples  
      tiling,                                             // tiling  
      usage,                                              // usage  
      sharing_mode,                                       // sharingMode  
      static_cast<uint32_t>(queue_family_indices.size()), // queueFamilyIndexCount
      queue_family_indices.data(),                        // pQueueFamilyIndices
      initial_layout,                                     // initialLayout
    };

    THROW_ON_ERROR(vkCreateImage(this->device->device, &create_info, nullptr, &this->image));
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
               const std::vector<uint32_t> & queueFamilyIndices = std::vector<uint32_t>())
    : device(device)
  {
    VkBufferCreateInfo create_info {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,             // sType  
      nullptr,                                          // pNext  
      flags,                                            // flags  
      size,                                             // size  
      usage,                                            // usage  
      sharingMode,                                      // sharingMode  
      static_cast<uint32_t>(queueFamilyIndices.size()), // queueFamilyIndexCount  
      queueFamilyIndices.data(),                        // pQueueFamilyIndices  
    };

    THROW_ON_ERROR(vkCreateBuffer(this->device->device, &create_info, nullptr, &this->buffer));
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
    VkAllocationCallbacks allocation_callbacks {
      nullptr,                     // pUserData  
      &VulkanMemory::allocate,     // pfnAllocation
      &VulkanMemory::reallocate,   // pfnReallocation
      &VulkanMemory::free,         // pfnFree
      nullptr,                     // pfnInternalAllocation
      nullptr,                     // pfnInternalFree
    };

    VkMemoryAllocateInfo allocate_info = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,                          // sType 
      nullptr,                                                         // pNext 
      requirements.size,                                               // allocationSize 
      device->physical_device.getMemoryTypeIndex(requirements, flags), // memoryTypeIndex 
    };

    THROW_ON_ERROR(vkAllocateMemory(this->device->device, &allocate_info, &allocation_callbacks, &this->memory));
  }

  VulkanMemory(const std::shared_ptr<VulkanDevice> & device, const std::unique_ptr<VulkanImage> & image, VkMemoryPropertyFlags flags)
    : VulkanMemory(device, image->memory_requirements, flags)
  {
    THROW_ON_ERROR(vkBindImageMemory(this->device->device, image->image, this->memory, 0));
  }

  VulkanMemory(const std::shared_ptr<VulkanDevice> & device, const std::unique_ptr<VulkanBuffer> & buffer, VkMemoryPropertyFlags flags)
    : VulkanMemory(device, buffer->memory_requirements, flags)
  {
    THROW_ON_ERROR(vkBindBufferMemory(this->device->device, buffer->buffer, this->memory, 0));
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
    THROW_ON_ERROR(vkMapMemory(this->device->device, this->memory, 0, size, 0, &data));
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
    VkImageViewCreateInfo create_info {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType 
      nullptr,                                  // pNext 
      0,                                        // flags (reserved for future use)
      image,                                    // image 
      view_type,                                // viewType 
      format,                                   // format 
      components,                               // components 
      subresource_range,                        // subresourceRange 
    };

    THROW_ON_ERROR(vkCreateImageView(this->device->device, &create_info, nullptr, &this->view));
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
    VkSamplerCreateInfo create_info {
      VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, // sType
      nullptr,                               // pNext
      0,                                     // flags (reserved for future use)
      magFilter,                             // magFilter
      minFilter,                             // minFilter
      mipmapMode,                            // mipmapMode
      addressModeU,                          // addressModeU
      addressModeV,                          // addressModeV
      addressModeW,                          // addressModeW
      mipLodBias,                            // mipLodBias
      anisotropyEnable,                      // anisotropyEnable
      maxAnisotropy,                         // maxAnisotropy
      compareEnable,                         // compareEnable
      compareOp,                             // compareOp
      minLod,                                // minLod
      maxLod,                                // maxLod
      borderColor,                           // borderColor
      unnormalizedCoordinates,               // unnormalizedCoordinates
    };

    THROW_ON_ERROR(vkCreateSampler(this->device->device, &create_info, nullptr, &this->sampler));
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
    VkShaderModuleCreateInfo create_info {
    VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,      // sType
      nullptr,                                        // pNext
      0,                                              // flags (reserved for future use)
      code.size(),                                    // codeSize
      reinterpret_cast<const uint32_t*>(code.data()), // pCode
    };

    THROW_ON_ERROR(vkCreateShaderModule(this->device->device, &create_info, nullptr, &this->module));
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
    VkPipelineCacheCreateInfo create_info {
      VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, // sType
      nullptr,                                      // pNext
      0,                                            // flags (reserved for future use)
      0,                                            // initialDataSize
      nullptr,                                      // pInitialData
    };

    THROW_ON_ERROR(vkCreatePipelineCache(this->device->device, &create_info, nullptr, &this->cache));
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
    VkRenderPassCreateInfo create_info {
    VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,    // sType
      nullptr,                                    // pNext
      0,                                          // flags (reserved for future use)
      static_cast<uint32_t>(attachments.size()),  // attachmentCount
      attachments.data(),                         // pAttachments
      static_cast<uint32_t>(subpasses.size()),    // subpassCount
      subpasses.data(),                           // pSubpasses
      static_cast<uint32_t>(dependencies.size()), // dependencyCount
      dependencies.data(),                        // pDependencies
    };

    THROW_ON_ERROR(vkCreateRenderPass(this->device->device, &create_info, nullptr, &this->renderpass));
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
    VkFramebufferCreateInfo create_info {
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags (reserved for future use)
      renderpass->renderpass,                    // renderPass
      static_cast<uint32_t>(attachments.size()), // attachmentCount
      attachments.data(),                        // pAttachments
      extent.width,                              // width
      extent.height,                             // height
      layers,                                    // layers
    };

    THROW_ON_ERROR(vkCreateFramebuffer(this->device->device, &create_info, nullptr, &this->framebuffer));
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
    VkPipelineLayoutCreateInfo create_info {
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,    // sType
      nullptr,                                          // pNext
      0,                                                // flags (reserved for future use)
      static_cast<uint32_t>(setlayouts.size()),         // setLayoutCount
      setlayouts.data(),                                // pSetLayouts
      static_cast<uint32_t>(pushconstantranges.size()), // pushConstantRangeCount
      pushconstantranges.data(),                        // pPushConstantRanges
    };

    THROW_ON_ERROR(vkCreatePipelineLayout(this->device->device, &create_info, nullptr, &this->layout));
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
    VkComputePipelineCreateInfo create_info {
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO, // sType
      nullptr,                                        // pNext
      flags,                                          // flags
      stage,                                          // stage
      layout,                                         // layout
      basePipelineHandle,                             // basePipelineHandle
      basePipelineIndex,                              // basePipelineIndex
    };

    THROW_ON_ERROR(vkCreateComputePipelines(this->device->device, pipelineCache, 1, &create_info, nullptr, &this->pipeline));
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
    VkPipelineVertexInputStateCreateInfo vertex_input_state {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags (reserved for future use)
      (uint32_t)binding_descriptions.size(),                     // vertexBindingDescriptionCount
      binding_descriptions.data(),                               // pVertexBindingDescriptions
      (uint32_t)attribute_descriptions.size(),                   // vertexAttributeDescriptionCount
      attribute_descriptions.data(),                             // pVertexAttributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state {
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, // sType
      nullptr,                                                     // pNext
      0,                                                           // flags (reserved for future use)
      primitive_topology,                                          // topology
      VK_FALSE,                                                    // primitiveRestartEnable
    };

    VkPipelineColorBlendAttachmentState blend_attachment_state {
      VK_TRUE,                                                                                                   // blendEnable
      VK_BLEND_FACTOR_SRC_COLOR,                                                                                 // srcColorBlendFactor
      VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,                                                                       // dstColorBlendFactor
      VK_BLEND_OP_ADD,                                                                                           // colorBlendOp
      VK_BLEND_FACTOR_SRC_ALPHA,                                                                                 // srcAlphaBlendFactor
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,                                                                       // dstAlphaBlendFactor
      VK_BLEND_OP_ADD,                                                                                           // alphaBlendOp
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, // colorWriteMask
    };

    VkPipelineColorBlendStateCreateInfo color_blend_state {
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, // sType 
      nullptr,                                                  // pNext
      0,                                                        // flags (reserved for future use)
      VK_FALSE,                                                 // logicOpEnable
      VK_LOGIC_OP_MAX_ENUM,                                     // logicOp
      1,                                                        // attachmentCount
      &blend_attachment_state,                                  // pAttachments  
      0,                                                        // blendConstants[4]
    };

    VkPipelineDynamicStateCreateInfo dynamic_state {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, // sType
      nullptr,                                              // pNext
      0,                                                    // flags (reserved for future use)
      (uint32_t)dynamic_states.size(),                      // dynamicStateCount
      dynamic_states.data(),                                // pDynamicStates
    };

    VkPipelineViewportStateCreateInfo viewport_state {
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, // sType
      nullptr,                                               // pNext
      0,                                                     // flags (reserved for future use)
      1,                                                     // viewportCount
      nullptr,                                               // pViewports
      1,                                                     // scissorCount
      nullptr,                                               // pScissors
    };

    VkStencilOpState stencil_op_state{
      VK_STENCIL_OP_KEEP,   // failOp
      VK_STENCIL_OP_KEEP,   // passOp
      VK_STENCIL_OP_KEEP,   // depthFailOp
      VK_COMPARE_OP_ALWAYS, // compareOp
      0,                    // compareMask
      0,                    // writeMask
      0,                    // reference
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state {
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO, // sType
      nullptr,                                                    // pNext
      0,                                                          // flags (reserved for future use)
      VK_TRUE,                                                    // depthTestEnable
      VK_TRUE,                                                    // depthWriteEnable
      VK_COMPARE_OP_LESS_OR_EQUAL,                                // depthCompareOp
      VK_FALSE,                                                   // depthBoundsTestEnable
      VK_FALSE,                                                   // stencilTestEnable
      stencil_op_state,                                           // front  
      stencil_op_state,                                           // back
      0,                                                          // minDepthBounds
      0,                                                          // maxDepthBounds
    };

    VkPipelineMultisampleStateCreateInfo multi_sample_state {
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, // sType 
      nullptr,                                                  // pNext
      0,                                                        // flags (reserved for future use)
      VK_SAMPLE_COUNT_1_BIT,                                    // rasterizationSamples
      VK_FALSE,                                                 // sampleShadingEnable
      0,                                                        // minSampleShading
      nullptr,                                                  // pSampleMask
      VK_FALSE,                                                 // alphaToCoverageEnable
      VK_FALSE,                                                 // alphaToOneEnable
    };

    VkGraphicsPipelineCreateInfo create_info {
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, // sType
      nullptr,                                         // pNext
      0,                                               // flags is a bitmask of VkPipelineCreateFlagBits specifying how the pipeline will be generated.
      (uint32_t)shaderstages.size(),                   // stageCount
      shaderstages.data(),                             // pStages
      &vertex_input_state,                             // pVertexInputState
      &input_assembly_state,                           // pInputAssemblyState
      nullptr,                                         // pTessellationState
      &viewport_state,                                 // pViewportState
      &rasterization_state,                            // pRasterizationState
      &multi_sample_state,                             // pMultisampleState
      &depth_stencil_state,                            // pDepthStencilState
      &color_blend_state,                              // pColorBlendState
      &dynamic_state,                                  // pDynamicState
      pipeline_layout,                                 // layout
      render_pass,                                     // renderPass
      0,                                               // subpass
      nullptr,                                         // basePipelineHandle
      0,                                               // basePipelineIndex
    };

    THROW_ON_ERROR(vkCreateGraphicsPipelines(this->device->device, pipeline_cache, 1, &create_info, nullptr, &this->pipeline));
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
