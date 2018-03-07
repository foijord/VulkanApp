#pragma once

#include <Innovator/Core/Misc/Defines.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>
#include <stdexcept>

class VkException: public std::exception {};
class VkTimeoutException : public VkException {};
class VkNotReadyException : public VkException {};
class VkIncompleteException : public VkException {};
class VkSuboptimalException : public VkException {};
class VkErrorOutOfDateException : public VkException {};
class VkErrorDeviceLostException : public VkException {};
class VkErrorSurfaceLostException : public VkException {};
class VkErrorNotPermittedException : public VkException {};
class VkErrorInvalidShaderException : public VkException {};
class VkErrorFragmentedPoolException : public VkException {};
class VkErrorTooManyObjectsException : public VkException {};
class VkErrorLayerNotPresentException : public VkException {};
class VkErrorMemoryMapFailedException : public VkException {};
class VkErrorOutOfHostMemoryException : public VkException {};
class VkErrorOutOfPoolMemoryException : public VkException {};
class VkErrorValidationFailedException : public VkException {};
class VkErrorNativeWindowInUseException : public VkException {};
class VkErrorFeatureNotPresentException : public VkException {};
class VkErrorOutOfDeviceMemoryException : public VkException {};
class VkErrorFormatNotSupportedException : public VkException {};
class VkErrorIncompatibleDriverException : public VkException {};
class VkErrorExtensionNotPresentException : public VkException {};
class VkErrorIncompatibleDisplayException : public VkException {};
class VkErrorInitializationFailedException : public VkException {};
class VkErrorInvalidExternalHandleException : public VkException {};

#define THROW_ON_ERROR(__function__) {                                                        \
	VkResult __result__ = (__function__);                                                       \
  switch (__result__) {                                                                       \
    case VK_TIMEOUT: throw VkTimeoutException();                                              \
    case VK_NOT_READY: throw VkNotReadyException();                                           \
    case VK_INCOMPLETE: throw VkIncompleteException();                                        \
    case VK_SUBOPTIMAL_KHR: throw VkSuboptimalException();                                    \
    case VK_ERROR_DEVICE_LOST: throw VkErrorDeviceLostException();                            \
    case VK_ERROR_OUT_OF_DATE_KHR: throw VkErrorOutOfDateException();                         \
    case VK_ERROR_SURFACE_LOST_KHR: throw VkErrorSurfaceLostException();                      \
    case VK_ERROR_NOT_PERMITTED_EXT: throw VkErrorNotPermittedException();                    \
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
    default: break;                                                                           \
  }                                                                                           \
}                                                                                             \

class VulkanPhysicalDevice {
public:
  VulkanPhysicalDevice(VulkanPhysicalDevice && self) = default;
  VulkanPhysicalDevice(const VulkanPhysicalDevice & self) = default;
  VulkanPhysicalDevice & operator=(VulkanPhysicalDevice && self) = delete;
  VulkanPhysicalDevice & operator=(const VulkanPhysicalDevice & self) = delete;

  explicit VulkanPhysicalDevice(VkPhysicalDevice device)
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

  ~VulkanPhysicalDevice() = default;

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
  
  bool supportsFeatures(const VkPhysicalDeviceFeatures & required_features)
  {
	  return
		  this->features.logicOp >= required_features.logicOp &&
		  this->features.wideLines >= required_features.wideLines &&
		  this->features.depthClamp >= required_features.depthClamp &&
		  this->features.alphaToOne >= required_features.alphaToOne &&
		  this->features.depthBounds >= required_features.depthBounds &&
		  this->features.largePoints >= required_features.largePoints &&
		  this->features.shaderInt64 >= required_features.shaderInt64 &&
		  this->features.shaderInt16 >= required_features.shaderInt16 &&
		  this->features.dualSrcBlend >= required_features.dualSrcBlend &&
		  this->features.multiViewport >= required_features.multiViewport &&
		  this->features.shaderFloat64 >= required_features.shaderFloat64 &&
		  this->features.sparseBinding >= required_features.sparseBinding &&
		  this->features.imageCubeArray >= required_features.imageCubeArray &&
		  this->features.geometryShader >= required_features.geometryShader &&
		  this->features.depthBiasClamp >= required_features.depthBiasClamp &&
		  this->features.independentBlend >= required_features.independentBlend &&
		  this->features.fillModeNonSolid >= required_features.fillModeNonSolid &&
		  this->features.inheritedQueries >= required_features.inheritedQueries &&
		  this->features.sampleRateShading >= required_features.sampleRateShading &&
		  this->features.multiDrawIndirect >= required_features.multiDrawIndirect &&
		  this->features.samplerAnisotropy >= required_features.samplerAnisotropy &&
		  this->features.robustBufferAccess >= required_features.robustBufferAccess &&
		  this->features.tessellationShader >= required_features.tessellationShader &&
		  this->features.shaderClipDistance >= required_features.shaderClipDistance &&
		  this->features.shaderCullDistance >= required_features.shaderCullDistance &&
		  this->features.fullDrawIndexUint32 >= required_features.fullDrawIndexUint32 &&
		  this->features.textureCompressionBC >= required_features.textureCompressionBC &&
		  this->features.shaderResourceMinLod >= required_features.shaderResourceMinLod &&
		  this->features.occlusionQueryPrecise >= required_features.occlusionQueryPrecise &&
		  this->features.sparseResidencyBuffer >= required_features.sparseResidencyBuffer &&
		  this->features.textureCompressionETC2 >= required_features.textureCompressionETC2 &&
		  this->features.sparseResidencyImage2D >= required_features.sparseResidencyImage2D &&
		  this->features.sparseResidencyImage3D >= required_features.sparseResidencyImage3D &&
		  this->features.sparseResidencyAliased >= required_features.sparseResidencyAliased &&
		  this->features.pipelineStatisticsQuery >= required_features.pipelineStatisticsQuery &&
		  this->features.shaderResourceResidency >= required_features.shaderResourceResidency &&
		  this->features.sparseResidency2Samples >= required_features.sparseResidency2Samples &&
		  this->features.sparseResidency4Samples >= required_features.sparseResidency4Samples &&
		  this->features.sparseResidency8Samples >= required_features.sparseResidency8Samples &&
		  this->features.variableMultisampleRate >= required_features.variableMultisampleRate &&
		  this->features.sparseResidency16Samples >= required_features.sparseResidency16Samples &&
		  this->features.fragmentStoresAndAtomics >= required_features.fragmentStoresAndAtomics &&
		  this->features.drawIndirectFirstInstance >= required_features.drawIndirectFirstInstance &&
		  this->features.shaderImageGatherExtended >= required_features.shaderImageGatherExtended &&
		  this->features.textureCompressionASTC_LDR >= required_features.textureCompressionASTC_LDR &&
		  this->features.shaderStorageImageMultisample >= required_features.shaderStorageImageMultisample &&
		  this->features.vertexPipelineStoresAndAtomics >= required_features.vertexPipelineStoresAndAtomics &&
		  this->features.shaderStorageImageExtendedFormats >= required_features.shaderStorageImageExtendedFormats &&
		  this->features.shaderStorageImageReadWithoutFormat >= required_features.shaderStorageImageReadWithoutFormat &&
		  this->features.shaderStorageImageWriteWithoutFormat >= required_features.shaderStorageImageWriteWithoutFormat &&
		  this->features.shaderTessellationAndGeometryPointSize >= required_features.shaderTessellationAndGeometryPointSize &&
		  this->features.shaderSampledImageArrayDynamicIndexing >= required_features.shaderSampledImageArrayDynamicIndexing &&
		  this->features.shaderStorageImageArrayDynamicIndexing >= required_features.shaderStorageImageArrayDynamicIndexing &&
		  this->features.shaderUniformBufferArrayDynamicIndexing >= required_features.shaderUniformBufferArrayDynamicIndexing &&
		  this->features.shaderStorageBufferArrayDynamicIndexing >= required_features.shaderStorageBufferArrayDynamicIndexing;
  }

  VkPhysicalDevice device;
  VkPhysicalDeviceFeatures features{};
  VkPhysicalDeviceProperties properties{};
  VkPhysicalDeviceMemoryProperties memory_properties{};
  std::vector<VkQueueFamilyProperties> queue_family_properties;
  std::vector<VkExtensionProperties> extension_properties;
  std::vector<VkLayerProperties> layer_properties;
};

class VulkanInstanceBase {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanInstanceBase);

  explicit VulkanInstanceBase(const VkApplicationInfo & application_info,
                              const std::vector<const char *> & required_layers,
                              const std::vector<const char *> & required_extensions)
    : instance(nullptr)
  {
    uint32_t layer_count;
    THROW_ON_ERROR(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
    std::vector<VkLayerProperties> layer_properties(layer_count);
    THROW_ON_ERROR(vkEnumerateInstanceLayerProperties(&layer_count, layer_properties.data()));

    std::for_each(required_layers.begin(), required_layers.end(), [&](const char * layer_name) {
      for (auto properties : layer_properties)
        if (std::strcmp(layer_name, properties.layerName) == 0)
          return;
      throw std::runtime_error("Required instance layer " + std::string(layer_name) + " not supported.");
    });

    uint32_t extension_count;
    THROW_ON_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));
    std::vector<VkExtensionProperties> extension_properties(extension_count);
    THROW_ON_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extension_properties.data()));

    std::for_each(required_extensions.begin(), required_extensions.end(), [&](const char * extension_name) {
      for (auto properties : extension_properties)
        if (std::strcmp(extension_name, properties.extensionName) == 0)
          return;
      throw std::runtime_error("Required instance extension " + std::string(extension_name) + " not supported.");
    });

    VkInstanceCreateInfo create_info {
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
  }

  ~VulkanInstanceBase()
  {
    vkDestroyInstance(this->instance, nullptr);
  }

  VkInstance instance;
};

class VulkanInstance : public VulkanInstanceBase {
public:
  VulkanInstance(const VkApplicationInfo & application_info,
                 const std::vector<const char *> & required_layers,
                 const std::vector<const char *> & required_extensions)
    : VulkanInstanceBase(application_info, required_layers, required_extensions)
  {
    uint32_t physical_device_count;
    THROW_ON_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    THROW_ON_ERROR(vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data()));

    for (const VkPhysicalDevice & physical_device : physical_devices) {
      this->physical_devices.push_back(VulkanPhysicalDevice(physical_device));
    }

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

  ~VulkanInstance() {}

  VulkanPhysicalDevice selectPhysicalDevice(const VkPhysicalDeviceFeatures & required_features) 
  {
    std::vector<VulkanPhysicalDevice> devices;
    for (VulkanPhysicalDevice & physical_device : this->physical_devices) {
      if (physical_device.supportsFeatures(required_features)) {
        devices.push_back(physical_device);
      }
    }
    if (devices.empty()) {
      throw std::runtime_error("Could not find physical device with the required features");
    }
    return devices.front();
  }

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
  NO_COPY_OR_ASSIGNMENT(VulkanLogicalDevice);

  VulkanLogicalDevice(const VulkanPhysicalDevice & physical_device,
                      const VkPhysicalDeviceFeatures & enabled_features,
                      const std::vector<const char *> & required_layers,
                      const std::vector<const char *> & required_extensions,
                      const std::vector<VkDeviceQueueCreateInfo> & queue_create_infos)
    : device(nullptr), physical_device(physical_device)
  {
    std::for_each(required_layers.begin(), required_layers.end(), [&](const char * layer_name) {
      for (auto properties : physical_device.layer_properties)
        if (std::strcmp(layer_name, properties.layerName) == 0)
          return;
      throw std::runtime_error("Required device layer " + std::string(layer_name) + " not supported.");
    });

    std::for_each(required_extensions.begin(), required_extensions.end(), [&](const char * extension_name) {
      for (auto properties : physical_device.extension_properties)
        if (std::strcmp(extension_name, properties.extensionName) == 0)
          return;
      throw std::runtime_error("Required device extension " + std::string(extension_name) + " not supported.");
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
  }

  ~VulkanLogicalDevice()
  {
    vkDestroyDevice(this->device, nullptr);
  }

  VkDevice device;
  VulkanPhysicalDevice physical_device;
};

class VulkanDevice : public VulkanLogicalDevice {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDevice);

  VulkanDevice(const VulkanPhysicalDevice & physical_device,
               const VkPhysicalDeviceFeatures & enabled_features,
               const std::vector<const char *> & required_layers,
               const std::vector<const char *> & required_extensions,
               const std::vector<VkDeviceQueueCreateInfo> & queue_create_infos)
    : VulkanLogicalDevice(physical_device, enabled_features, required_layers, required_extensions, queue_create_infos),
      default_queue(nullptr),
      default_pool(nullptr),
      default_queue_index(0)
 {
    this->default_queue_index = queue_create_infos[0].queueFamilyIndex;
    vkGetDeviceQueue(this->device, this->default_queue_index, 0, &this->default_queue);

    VkCommandPoolCreateInfo create_info {
      VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,       // sType
      nullptr,                                          // pNext 
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  // flags
      this->default_queue_index,                        // queueFamilyIndex 
    };

    THROW_ON_ERROR(vkCreateCommandPool(this->device, &create_info, nullptr, &this->default_pool));
  }

  ~VulkanDevice() 
  {
    vkDestroyCommandPool(this->device, this->default_pool, nullptr);
  }

  VkQueue default_queue;
  VkCommandPool default_pool;
  uint32_t default_queue_index;
};

class VulkanDebugCallback {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDebugCallback);

  explicit VulkanDebugCallback(std::shared_ptr<VulkanInstance> vulkan,
                               VkDebugReportFlagsEXT flags,
                               PFN_vkDebugReportCallbackEXT callback,
                               void * pUserData = nullptr)
    : vulkan(std::move(vulkan)),
      callback(nullptr)
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

  std::shared_ptr<VulkanInstance> vulkan;
  VkDebugReportCallbackEXT callback;
};

class VulkanSemaphore {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSemaphore);

  VulkanSemaphore(std::shared_ptr<VulkanDevice> device)
    : device(std::move(device))
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
  NO_COPY_OR_ASSIGNMENT(VulkanSwapchain);

  VulkanSwapchain(std::shared_ptr<VulkanDevice> device,
                  std::shared_ptr<VulkanInstance> vulkan,
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
    : vulkan(std::move(vulkan)), 
      device(std::move(device)),
      swapchain(nullptr)
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

  std::vector<VkImage> getSwapchainImages() const
  {
    uint32_t count;
    THROW_ON_ERROR(vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &count, nullptr));
    std::vector<VkImage> images(count);
    THROW_ON_ERROR(vulkan->vkGetSwapchainImages(this->device->device, this->swapchain, &count, images.data()));
    return images;
  }

  uint32_t getNextImageIndex(const std::shared_ptr<VulkanSemaphore> & semaphore) const
  {
    uint32_t index;
    THROW_ON_ERROR(this->vulkan->vkAcquireNextImage(this->device->device, this->swapchain, UINT64_MAX, semaphore->semaphore, nullptr, &index));
    return index;
  };


  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanInstance> vulkan;
  VkSwapchainKHR swapchain;
};

class VulkanDescriptorPool {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDescriptorPool);

  VulkanDescriptorPool(std::shared_ptr<VulkanDevice> device, 
                       std::vector<VkDescriptorPoolSize> descriptor_pool_sizes)
    : device(std::move(device)),
      pool(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkDescriptorPool pool;
};

class VulkanDescriptorSetLayout {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDescriptorSetLayout);

  VulkanDescriptorSetLayout(std::shared_ptr<VulkanDevice> device, 
                            std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings)
    : device(std::move(device)),
      layout(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkDescriptorSetLayout layout;
};

class VulkanDescriptorSets {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDescriptorSets);

  VulkanDescriptorSets(std::shared_ptr<VulkanDevice> device,
                       std::shared_ptr<VulkanDescriptorPool> pool,
                       uint32_t count,
                       VkDescriptorSetLayout descriptor_set_layout)
    : device(std::move(device)), 
      pool(std::move(pool))
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
              const std::vector<VkCopyDescriptorSet> & descriptor_copies = std::vector<VkCopyDescriptorSet>()) const
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
  NO_COPY_OR_ASSIGNMENT(VulkanFence);

  explicit VulkanFence(std::shared_ptr<VulkanDevice> device)
    : device(std::move(device)),
      fence(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkFence fence;
};

class VulkanCommandBuffers {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanCommandBuffers);

  explicit VulkanCommandBuffers(std::shared_ptr<VulkanDevice> device, 
                                size_t count = 1,
                                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    : device(std::move(device))
  {
    VkCommandBufferAllocateInfo allocate_info {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType 
      nullptr,                                        // pNext 
      this->device->default_pool,                     // commandPool 
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
};

class VulkanImage {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanImage);

  VulkanImage(std::shared_ptr<VulkanDevice> device, 
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
    : device(std::move(device)),
      image(nullptr)
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
  }

  ~VulkanImage()
  {
    vkDestroyImage(this->device->device, this->image, nullptr);
  }

  std::shared_ptr<VulkanDevice> device;
  VkImage image;
};

class VulkanBuffer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanBuffer);

  VulkanBuffer(std::shared_ptr<VulkanDevice> device,
               VkBufferCreateFlags flags, 
               VkDeviceSize size, 
               VkBufferUsageFlags usage,
               VkSharingMode sharingMode,
               const std::vector<uint32_t> & queueFamilyIndices = std::vector<uint32_t>())
    : device(std::move(device)),
      buffer(nullptr)
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
  }

  ~VulkanBuffer()
  {
    vkDestroyBuffer(this->device->device, this->buffer, nullptr);
  }

  std::shared_ptr<VulkanDevice> device;
  VkBuffer buffer;
};

class VulkanMemory {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanMemory);
  
  VulkanMemory(std::shared_ptr<VulkanDevice> device, 
               VkMemoryRequirements requirements, 
               VkMemoryPropertyFlags flags)
    : device(std::move(device)),
      memory(nullptr)
  {
    VkAllocationCallbacks allocation_callbacks {
      nullptr,                     // pUserData  
      &VulkanMemory::allocate,     // pfnAllocation
      &VulkanMemory::reallocate,   // pfnReallocation
      &VulkanMemory::free,         // pfnFree
      nullptr,                     // pfnInternalAllocation
      nullptr,                     // pfnInternalFree
    };

    VkMemoryAllocateInfo allocate_info {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,                                 // sType 
      nullptr,                                                                // pNext 
      requirements.size,                                                      // allocationSize 
      this->device->physical_device.getMemoryTypeIndex(requirements, flags),  // memoryTypeIndex 
    };

    THROW_ON_ERROR(vkAllocateMemory(this->device->device, &allocate_info, &allocation_callbacks, &this->memory));
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

  class MemoryCopy {
  public:
    NO_COPY_OR_ASSIGNMENT(MemoryCopy);

    MemoryCopy(VulkanMemory * memory, const void * src, size_t size)
      : memory(memory) {
      ::memcpy(this->memory->map(size), src, size);
    }
    ~MemoryCopy() {
      this->memory->unmap();
    }
    VulkanMemory * memory;
  };

  void memcpy(const void * src, size_t size)
  {
    MemoryCopy copy(this, src, size);
  }

  std::shared_ptr<VulkanDevice> device;
  VkDeviceMemory memory;
};

class VulkanImageMemory : public VulkanMemory {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanImageMemory);

  VulkanImageMemory(const std::shared_ptr<VulkanImage> & image, 
                    const VkMemoryRequirements & memory_requirements, 
                    VkMemoryPropertyFlags flags)
    : VulkanMemory(image->device, memory_requirements, flags)
  {
    THROW_ON_ERROR(vkBindImageMemory(image->device->device, image->image, this->memory, 0));
  }

  ~VulkanImageMemory() = default;
};

class VulkanBufferMemory : public VulkanMemory {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanBufferMemory);

  VulkanBufferMemory(const std::shared_ptr<VulkanBuffer> & buffer, 
                     const VkMemoryRequirements & memory_requirements,
                     VkMemoryPropertyFlags flags)
    : VulkanMemory(buffer->device, memory_requirements, flags)
  {
    THROW_ON_ERROR(vkBindBufferMemory(buffer->device->device, buffer->buffer, this->memory, 0));
  }

  ~VulkanBufferMemory() = default;
};

class VulkanImageView {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanImageView);

  VulkanImageView(std::shared_ptr<VulkanImage> image,
                  VkFormat format,
                  VkImageViewType view_type, 
                  VkComponentMapping components,
                  VkImageSubresourceRange subresource_range)
    : image(std::move(image)),
      view(nullptr)
  {
    VkImageViewCreateInfo create_info {
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType 
      nullptr,                                  // pNext 
      0,                                        // flags (reserved for future use)
      this->image->image,                       // image 
      view_type,                                // viewType 
      format,                                   // format 
      components,                               // components 
      subresource_range,                        // subresourceRange 
    };

    THROW_ON_ERROR(vkCreateImageView(this->image->device->device, &create_info, nullptr, &this->view));
  }

  ~VulkanImageView()
  {
    vkDestroyImageView(this->image->device->device, this->view, nullptr);
  }

  std::shared_ptr<VulkanImage> image;
  VkImageView view;
};

class VulkanSampler {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSampler);

  VulkanSampler(std::shared_ptr<VulkanDevice> device, 
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
    : device(std::move(device)),
      sampler(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkSampler sampler;
};

class VulkanShaderModule {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanShaderModule);

  VulkanShaderModule(std::shared_ptr<VulkanDevice> device,
                     const std::vector<char> & code)
    : device(std::move(device)),
      module(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkShaderModule module;
};


class VulkanPipelineCache {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanPipelineCache);

  explicit VulkanPipelineCache(std::shared_ptr<VulkanDevice> device)
    : device(std::move(device)),
      cache(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkPipelineCache cache;
};

class VulkanRenderpass {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanRenderpass);

  VulkanRenderpass(std::shared_ptr<VulkanDevice> device,
                   const std::vector<VkAttachmentDescription> & attachments,
                   const std::vector<VkSubpassDescription> & subpasses,
                   const std::vector<VkSubpassDependency> & dependencies = std::vector<VkSubpassDependency>())
    : device(std::move(device)),
      renderpass(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkRenderPass renderpass;
};


class VulkanFramebuffer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanFramebuffer);

  VulkanFramebuffer(std::shared_ptr<VulkanDevice> device,
                    const std::shared_ptr<VulkanRenderpass> & renderpass,
                    std::vector<VkImageView> attachments, 
                    VkExtent2D extent,
                    uint32_t layers)
    : device(std::move(device)),
      framebuffer(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkFramebuffer framebuffer;
};

class VulkanPipelineLayout {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanPipelineLayout);

  VulkanPipelineLayout(std::shared_ptr<VulkanDevice> device, 
                       const std::vector<VkDescriptorSetLayout> & setlayouts,
                       const std::vector<VkPushConstantRange> & pushconstantranges = std::vector<VkPushConstantRange>())
    : device(std::move(device)),
      layout(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkPipelineLayout layout;
};

class VulkanComputePipeline {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanComputePipeline);

  VulkanComputePipeline(std::shared_ptr<VulkanDevice> device,
                        VkPipelineCache pipelineCache,
                        VkPipelineCreateFlags flags,
                        const VkPipelineShaderStageCreateInfo & stage,
                        VkPipelineLayout layout,
                        VkPipeline basePipelineHandle = nullptr,
                        int32_t basePipelineIndex = 0)
    : device(std::move(device)),
      pipeline(nullptr)
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

  std::shared_ptr<VulkanDevice> device;
  VkPipeline pipeline;
};

class VulkanGraphicsPipeline {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanGraphicsPipeline);

  VulkanGraphicsPipeline(std::shared_ptr<VulkanDevice> device, 
                         VkRenderPass render_pass,
                         VkPipelineCache pipeline_cache,
                         VkPipelineLayout pipeline_layout,
                         VkPrimitiveTopology primitive_topology,
                         VkPipelineRasterizationStateCreateInfo rasterization_state,
                         std::vector<VkDynamicState> dynamic_states,
                         std::vector<VkPipelineShaderStageCreateInfo> shaderstages,
                         std::vector<VkVertexInputBindingDescription> binding_descriptions,
                         std::vector<VkVertexInputAttributeDescription> attribute_descriptions)
    : device(std::move(device)),
      pipeline(nullptr)
  {
    VkPipelineVertexInputStateCreateInfo vertex_input_state {
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, // sType
      nullptr,                                                   // pNext
      0,                                                         // flags (reserved for future use)
      static_cast<uint32_t>(binding_descriptions.size()),        // vertexBindingDescriptionCount
      binding_descriptions.data(),                               // pVertexBindingDescriptions
      static_cast<uint32_t>(attribute_descriptions.size()),      // vertexAttributeDescriptionCount
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
      static_cast<uint32_t>(dynamic_states.size()),         // dynamicStateCount
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
      static_cast<uint32_t>(shaderstages.size()),      // stageCount
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

  std::shared_ptr<VulkanDevice> device;
  VkPipeline pipeline;
};
