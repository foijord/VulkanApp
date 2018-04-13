#pragma once

#include <Innovator/Core/Misc/Defines.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <vector>
#include <memory>
#include <iostream>
#include <algorithm>

class VulkanPhysicalDevice {
public:
  VulkanPhysicalDevice(VulkanPhysicalDevice && self) = default;
  VulkanPhysicalDevice(const VulkanPhysicalDevice & self) = default;
  VulkanPhysicalDevice & operator=(VulkanPhysicalDevice && self) = delete;
  VulkanPhysicalDevice & operator=(const VulkanPhysicalDevice & self) = delete;
  ~VulkanPhysicalDevice() = default;

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

  uint32_t getMemoryTypeIndex(uint32_t memory_type, VkMemoryPropertyFlags required_flags) const
  {
    for (uint32_t i = 0; i < VK_MAX_MEMORY_TYPES; i++) {
      if (((memory_type >> i) & 1) == 1) { // check for required memory type
        if ((this->memory_properties.memoryTypes[i].propertyFlags & required_flags) == required_flags) {
          return i;
        }
      }
    }
    throw std::runtime_error("VulkanDevice::getQueueIndex: could not find suitable memory type");
  }
  
  bool supportsFeatures(const VkPhysicalDeviceFeatures & required_features) const
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
  NO_COPY_OR_ASSIGNMENT(VulkanInstanceBase)
  VulkanInstanceBase() = delete;

  explicit VulkanInstanceBase(const VkApplicationInfo & application_info,
                              const std::vector<const char *> & required_layers,
                              const std::vector<const char *> & required_extensions)
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

  VkInstance instance{ nullptr };
};

class VulkanInstance : public VulkanInstanceBase {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanInstance)
  VulkanInstance() = delete;
  ~VulkanInstance() = default;

  explicit VulkanInstance(const VkApplicationInfo & application_info,
                          const std::vector<const char *> & required_layers,
                          const std::vector<const char *> & required_extensions) :
    VulkanInstanceBase(application_info, required_layers, required_extensions),
    vkQueuePresent(getProcAddress<PFN_vkQueuePresentKHR>("vkQueuePresentKHR")),
    vkCreateSwapchain(getProcAddress<PFN_vkCreateSwapchainKHR>("vkCreateSwapchainKHR")),
    vkAcquireNextImage(getProcAddress<PFN_vkAcquireNextImageKHR>("vkAcquireNextImageKHR")),
    vkDestroySwapchain(getProcAddress<PFN_vkDestroySwapchainKHR>("vkDestroySwapchainKHR")),
    vkGetSwapchainImages(getProcAddress<PFN_vkGetSwapchainImagesKHR>("vkGetSwapchainImagesKHR")),
    vkCreateDebugReportCallback(getProcAddress<PFN_vkCreateDebugReportCallbackEXT>("vkCreateDebugReportCallbackEXT")),
    vkDestroyDebugReportCallback(getProcAddress<PFN_vkDestroyDebugReportCallbackEXT>("vkDestroyDebugReportCallbackEXT")),
    vkGetPhysicalDeviceSurfaceSupport(getProcAddress<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>("vkGetPhysicalDeviceSurfaceSupportKHR")),
    vkGetPhysicalDeviceSurfaceFormats(getProcAddress<PFN_vkGetPhysicalDeviceSurfaceFormatsKHR>("vkGetPhysicalDeviceSurfaceFormatsKHR")),
    vkGetPhysicalDeviceSurfaceCapabilities(getProcAddress<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>("vkGetPhysicalDeviceSurfaceCapabilitiesKHR")),
    vkGetPhysicalDeviceSurfacePresentModes(getProcAddress<PFN_vkGetPhysicalDeviceSurfacePresentModesKHR>("vkGetPhysicalDeviceSurfacePresentModesKHR"))
  {
    uint32_t physical_device_count;
    THROW_ON_ERROR(vkEnumeratePhysicalDevices(this->instance, &physical_device_count, nullptr));

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    THROW_ON_ERROR(vkEnumeratePhysicalDevices(this->instance, &physical_device_count, physical_devices.data()));

    for (const VkPhysicalDevice & physical_device : physical_devices) {
      this->physical_devices.emplace_back(physical_device);
    }
  }

  template <typename T>
  T getProcAddress(const std::string & name) {
    auto address = reinterpret_cast<T>(vkGetInstanceProcAddr(this->instance, name.c_str()));
    if (!address) {
      throw std::runtime_error("vkGetInstanceProcAddr failed for " + name);
    }
    return address;
  };

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
  NO_COPY_OR_ASSIGNMENT(VulkanLogicalDevice)
  VulkanLogicalDevice() = delete;

  VulkanLogicalDevice(const VulkanPhysicalDevice & physical_device,
                      const VkPhysicalDeviceFeatures & enabled_features,
                      const std::vector<const char *> & required_layers,
                      const std::vector<const char *> & required_extensions,
                      const std::vector<VkDeviceQueueCreateInfo> & queue_create_infos)
    : physical_device(physical_device)
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

  virtual ~VulkanLogicalDevice()
  {
    vkDestroyDevice(this->device, nullptr);
  }

  VkDevice device{ nullptr };
  VulkanPhysicalDevice physical_device;
};

class VulkanDevice;

class VulkanMemory {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanMemory)
  VulkanMemory() = delete;

  explicit VulkanMemory(std::shared_ptr<VulkanDevice> device,
                        VkDeviceSize size,
                        uint32_t memory_type_index);

  ~VulkanMemory();

  void* map(VkDeviceSize size, VkDeviceSize offset, VkMemoryMapFlags flags = 0) const;
  void unmap() const;
  void memcpy(const void* src, VkDeviceSize size, VkDeviceSize offset);

  std::shared_ptr<VulkanDevice> device;
  VkDeviceMemory memory{ nullptr };
};

class VulkanDevice : public VulkanLogicalDevice {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDevice)
  VulkanDevice() = delete;

  VulkanDevice(const VulkanPhysicalDevice& physical_device,
               const VkPhysicalDeviceFeatures& enabled_features,
               const std::vector<const char *>& required_layers,
               const std::vector<const char *>& required_extensions,
               const std::vector<VkDeviceQueueCreateInfo>& queue_create_infos);

  virtual ~VulkanDevice();

  VkQueue default_queue{ nullptr };
  uint32_t default_queue_index{ 0 };
  VkCommandPool default_pool{ nullptr };
};

inline 
VulkanDevice::VulkanDevice(const VulkanPhysicalDevice& physical_device,
                           const VkPhysicalDeviceFeatures& enabled_features,
                           const std::vector<const char*>& required_layers,
                           const std::vector<const char*>& required_extensions,
                           const std::vector<VkDeviceQueueCreateInfo>& queue_create_infos)
: VulkanLogicalDevice(physical_device, enabled_features, required_layers, required_extensions, queue_create_infos)
{
  this->default_queue_index = queue_create_infos[0].queueFamilyIndex;
  vkGetDeviceQueue(this->device, this->default_queue_index, 0, &this->default_queue);

  VkCommandPoolCreateInfo create_info{
    VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,       // sType
    nullptr,                                          // pNext 
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,  // flags
    this->default_queue_index,                        // queueFamilyIndex 
  };

  THROW_ON_ERROR(vkCreateCommandPool(this->device, &create_info, nullptr, &this->default_pool));
}

inline 
VulkanDevice::~VulkanDevice()
{
  vkDestroyCommandPool(this->device, this->default_pool, nullptr);
}

class VulkanDebugCallback {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDebugCallback)
  VulkanDebugCallback() = delete;

  explicit VulkanDebugCallback(std::shared_ptr<VulkanInstance> vulkan,
                               VkDebugReportFlagsEXT flags,
                               PFN_vkDebugReportCallbackEXT callback,
                               void * userdata = nullptr)
    : vulkan(std::move(vulkan))
  {
    VkDebugReportCallbackCreateInfoEXT create_info {
      VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT, // sType
      nullptr,                                        // pNext  
      flags,                                          // flags  
      callback,                                       // pfnCallback  
      userdata,                                       // pUserData 
    };

    THROW_ON_ERROR(this->vulkan->vkCreateDebugReportCallback(this->vulkan->instance, &create_info, nullptr, &this->callback));
  }

  ~VulkanDebugCallback()
  {
    this->vulkan->vkDestroyDebugReportCallback(this->vulkan->instance, this->callback, nullptr);
  }

  std::shared_ptr<VulkanInstance> vulkan;
  VkDebugReportCallbackEXT callback { nullptr };
};

class VulkanSemaphore {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSemaphore)
  VulkanSemaphore() = delete;

  explicit VulkanSemaphore(std::shared_ptr<VulkanDevice> device)
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

  VkSemaphore semaphore{ nullptr };
  std::shared_ptr<VulkanDevice> device;
};

class VulkanSwapchain {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSwapchain)
  VulkanSwapchain() = delete;

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
      device(std::move(device))
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
    THROW_ON_ERROR(this->vulkan->vkAcquireNextImage(this->device->device, 
                                                    this->swapchain, 
                                                    UINT64_MAX, 
                                                    semaphore->semaphore, 
                                                    nullptr, 
                                                    &index));
    return index;
  };

  std::shared_ptr<VulkanInstance> vulkan;
  std::shared_ptr<VulkanDevice> device;
  VkSwapchainKHR swapchain { nullptr };
};

class VulkanDescriptorPool {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDescriptorPool)
  VulkanDescriptorPool() = delete;

  explicit VulkanDescriptorPool(std::shared_ptr<VulkanDevice> device, 
                                std::vector<VkDescriptorPoolSize> descriptor_pool_sizes)
    : device(std::move(device))
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
  VkDescriptorPool pool { nullptr };
};

class VulkanDescriptorSetLayout {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDescriptorSetLayout)
  VulkanDescriptorSetLayout() = delete;

  VulkanDescriptorSetLayout(std::shared_ptr<VulkanDevice> device, 
                            std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings)
    : device(std::move(device))
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
  VkDescriptorSetLayout layout { nullptr };
};

class VulkanDescriptorSets {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanDescriptorSets)
  VulkanDescriptorSets() = delete;

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

  std::shared_ptr<VulkanDevice> device;
  std::shared_ptr<VulkanDescriptorPool> pool;
  std::vector<VkDescriptorSet> descriptor_sets;
};

class VulkanFence {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanFence)
  VulkanFence() = delete;

  explicit VulkanFence(std::shared_ptr<VulkanDevice> device)
    : device(std::move(device))
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
  VkFence fence { nullptr };
};

class FenceScope {
public:
  NO_COPY_OR_ASSIGNMENT(FenceScope)
  FenceScope() = delete;

  explicit FenceScope(VkDevice device, VkFence fence) :
    device(device),
    fence(fence)
  {
    THROW_ON_ERROR(vkResetFences(this->device, 1, &this->fence));
  }

  ~FenceScope()
  {
    try {
      THROW_ON_ERROR(vkWaitForFences(this->device, 1, &this->fence, VK_TRUE, UINT64_MAX));
    } catch(std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  VkDevice device;
  VkFence fence;
};

class VulkanCommandBuffers {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanCommandBuffers)
  VulkanCommandBuffers() = delete;

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

  void submit(VkQueue queue, 
              VkPipelineStageFlags flags, 
              size_t buffer_index, 
              const std::vector<VkSemaphore> & wait_semaphores = std::vector<VkSemaphore>(),
              const std::vector<VkSemaphore> & signal_semaphores = std::vector<VkSemaphore>(),
              VkFence fence = VK_NULL_HANDLE)
  {
    submit(queue, flags, { this->buffers[buffer_index] }, wait_semaphores, signal_semaphores, fence);
  }

  void submit(VkQueue queue,
              VkPipelineStageFlags flags,
              VkFence fence = VK_NULL_HANDLE,
              const std::vector<VkSemaphore> & wait_semaphores = std::vector<VkSemaphore>(),
              const std::vector<VkSemaphore> & signal_semaphores = std::vector<VkSemaphore>())
  {
    submit(queue, flags, this->buffers, wait_semaphores, signal_semaphores, fence);
  }

  static void submit(VkQueue queue,
                     VkPipelineStageFlags flags,
                     const std::vector<VkCommandBuffer> & buffers,
                     const std::vector<VkSemaphore> & wait_semaphores,
                     const std::vector<VkSemaphore> & signal_semaphores,
                     VkFence fence)
  {
    VkSubmitInfo submit_info{
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

class VulkanCommandBufferScope {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanCommandBufferScope)
  VulkanCommandBufferScope() = delete;

  VulkanCommandBufferScope(VkCommandBuffer command,
                           VkRenderPass renderpass = nullptr,
                           uint32_t subpass = 0,
                           VkFramebuffer framebuffer = nullptr,
                           VkCommandBufferUsageFlags flags = 0) :
    command(command)
  {
    VkCommandBufferInheritanceInfo inheritance_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO, // sType
      nullptr,                                           // pNext
      renderpass,                                        // renderPass 
      subpass,                                           // subpass 
      framebuffer,                                       // framebuffer
      VK_FALSE,                                          // occlusionQueryEnable
      0,                                                 // queryFlags 
      0,                                                 // pipelineStatistics 
    };

    VkCommandBufferBeginInfo begin_info{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, // sType 
      nullptr,                                     // pNext 
      flags,                                       // flags 
      &inheritance_info,                           // pInheritanceInfo 
    };

    THROW_ON_ERROR(vkBeginCommandBuffer(this->command, &begin_info));
  }

  ~VulkanCommandBufferScope()
  {
    try {
      THROW_ON_ERROR(vkEndCommandBuffer(this->command));
    } catch (std::exception & e) {
      std::cerr << e.what() << std::endl;
    }
  }

  VkCommandBuffer command;
};

class VulkanImage {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanImage)
  VulkanImage() = delete;

  VulkanImage(std::shared_ptr<VulkanDevice> device, 
              VkImageType image_type, 
              VkFormat format, 
              VkExtent3D extent,
              uint32_t mip_levels,
              uint32_t array_layers,
              VkSampleCountFlagBits samples,
              VkImageTiling tiling, 
              VkImageUsageFlags usage, 
              VkSharingMode sharing_mode,
              VkImageCreateFlags flags = 0,
              std::vector<uint32_t> queue_family_indices = std::vector<uint32_t>(),
              VkImageLayout initial_layout = VK_IMAGE_LAYOUT_UNDEFINED)
    : device(std::move(device))
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
  VkImage image{ nullptr };;
};

class VulkanBuffer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanBuffer)
  VulkanBuffer() = delete;

  VulkanBuffer(std::shared_ptr<VulkanDevice> device,
               VkBufferCreateFlags flags, 
               VkDeviceSize size, 
               VkBufferUsageFlags usage,
               VkSharingMode sharingMode,
               const std::vector<uint32_t> & queueFamilyIndices = std::vector<uint32_t>())
    : device(std::move(device))
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
  VkBuffer buffer{ nullptr };
};

inline 
VulkanMemory::VulkanMemory(std::shared_ptr<VulkanDevice> device, 
                           VkDeviceSize size,
                           uint32_t memory_type_index)
  : device(std::move(device))
{
  VkMemoryAllocateInfo allocate_info{
    VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType 
    nullptr,                                // pNext 
    size,                                   // allocationSize 
    memory_type_index,                      // memoryTypeIndex 
  };

  THROW_ON_ERROR(vkAllocateMemory(this->device->device, &allocate_info, nullptr, &this->memory));
}

inline 
VulkanMemory::~VulkanMemory()
{
  vkFreeMemory(this->device->device, this->memory, nullptr);
}

inline void* 
VulkanMemory::map(VkDeviceSize size, VkDeviceSize offset, VkMemoryMapFlags flags) const
{
  void* data;
  THROW_ON_ERROR(vkMapMemory(this->device->device, this->memory, offset, size, flags, &data));
  return data;
}

inline void 
VulkanMemory::unmap() const
{
  vkUnmapMemory(this->device->device, this->memory);
}

class MemoryCopy {
public:
  NO_COPY_OR_ASSIGNMENT(MemoryCopy)

  MemoryCopy(VulkanMemory * memory, const void * src, VkDeviceSize size, VkDeviceSize offset)
    : memory(memory)
  {
    ::memcpy(this->memory->map(size, offset), src, size);
  }
  ~MemoryCopy() {
    this->memory->unmap();
  }
  VulkanMemory * memory{ nullptr };
};

inline void 
VulkanMemory::memcpy(const void* src, VkDeviceSize size, VkDeviceSize offset)
{
  MemoryCopy copy(this, src, size, offset);
}

class VulkanImageView {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanImageView)
  VulkanImageView() = delete;

  VulkanImageView(std::shared_ptr<VulkanImage> image,
                  VkFormat format,
                  VkImageViewType view_type, 
                  VkComponentMapping components,
                  VkImageSubresourceRange subresource_range)
    : image(std::move(image))
  {
    VkImageViewCreateInfo create_info{
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
  VkImageView view{ nullptr };
};

class VulkanSampler {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanSampler)
  VulkanSampler() = delete;

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
    : device(std::move(device))
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
  VkSampler sampler{ nullptr };
};

class VulkanShaderModule {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanShaderModule)
  VulkanShaderModule() = delete;

  VulkanShaderModule(std::shared_ptr<VulkanDevice> device,
                     const std::vector<char> & code)
    : device(std::move(device))
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
  VkShaderModule module{ nullptr };
};


class VulkanPipelineCache {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanPipelineCache)
  VulkanPipelineCache() = delete;

  explicit VulkanPipelineCache(std::shared_ptr<VulkanDevice> device)
    : device(std::move(device))
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
  VkPipelineCache cache{ nullptr };
};

class VulkanRenderpass {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanRenderpass)
  VulkanRenderpass() = delete;

  VulkanRenderpass(std::shared_ptr<VulkanDevice> device,
                   const std::vector<VkAttachmentDescription> & attachments,
                   const std::vector<VkSubpassDescription> & subpasses,
                   const std::vector<VkSubpassDependency> & dependencies = std::vector<VkSubpassDependency>())
    : device(std::move(device))
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
  VkRenderPass renderpass{ nullptr };
};

class VulkanRenderPassScope {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanRenderPassScope)
  VulkanRenderPassScope() = delete;

  explicit VulkanRenderPassScope(VkRenderPass renderpass,
                                 VkFramebuffer framebuffer,
                                 const VkRect2D renderarea,
                                 const std::vector<VkClearValue> & clearvalues,
                                 VkCommandBuffer command) :
    command(command)
  {
    VkRenderPassBeginInfo begin_info{
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,        // sType
      nullptr,                                         // pNext
      renderpass,                                      // renderPass
      framebuffer,                                     // framebuffer
      renderarea,                                      // renderArea
      static_cast<uint32_t>(clearvalues.size()),       // clearValueCount
      clearvalues.data()                               // pClearValues
    };

    vkCmdBeginRenderPass(this->command,
                         &begin_info,
                         VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
  }
    
  ~VulkanRenderPassScope()
  {
    vkCmdEndRenderPass(this->command);
  }

  VkCommandBuffer command;
};

class VulkanFramebuffer {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanFramebuffer)
  VulkanFramebuffer() = delete;

  VulkanFramebuffer(std::shared_ptr<VulkanDevice> device,
                    std::shared_ptr<VulkanRenderpass> renderpass,
                    const std::vector<VkImageView> & attachments,
                    VkExtent2D extent,
                    uint32_t layers) : 
    device(std::move(device)),
    renderpass(std::move(renderpass))
  {
    VkFramebufferCreateInfo create_info{
      VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
      nullptr,                                   // pNext
      0,                                         // flags (reserved for future use)
      this->renderpass->renderpass,              // renderPass
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
  std::shared_ptr<VulkanRenderpass> renderpass;
  VkFramebuffer framebuffer{ nullptr };
};

class VulkanPipelineLayout {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanPipelineLayout)
  VulkanPipelineLayout() = delete;

  VulkanPipelineLayout(std::shared_ptr<VulkanDevice> device, 
                       const std::vector<VkDescriptorSetLayout> & setlayouts,
                       const std::vector<VkPushConstantRange> & pushconstantranges = std::vector<VkPushConstantRange>())
    : device(std::move(device))
  {
    VkPipelineLayoutCreateInfo create_info{
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
  VkPipelineLayout layout{ nullptr };
};

class VulkanComputePipeline {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanComputePipeline)
  VulkanComputePipeline() = delete;

  VulkanComputePipeline(std::shared_ptr<VulkanDevice> device,
                        VkPipelineCache pipelineCache,
                        VkPipelineCreateFlags flags,
                        const VkPipelineShaderStageCreateInfo & stage,
                        VkPipelineLayout layout,
                        VkPipeline basePipelineHandle = nullptr,
                        int32_t basePipelineIndex = 0)
    : device(std::move(device))
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
  VkPipeline pipeline{ nullptr };
};

class VulkanGraphicsPipeline {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanGraphicsPipeline)
  VulkanGraphicsPipeline() = delete;

  VulkanGraphicsPipeline(std::shared_ptr<VulkanDevice> device, 
                         VkRenderPass render_pass,
                         VkPipelineCache pipeline_cache,
                         VkPipelineLayout pipeline_layout,
                         VkPrimitiveTopology primitive_topology,
                         VkPipelineRasterizationStateCreateInfo rasterization_state,
                         const std::vector<VkDynamicState> & dynamic_states,
                         const std::vector<VkPipelineShaderStageCreateInfo> & shaderstages,
                         const std::vector<VkVertexInputBindingDescription> & binding_descriptions,
                         const std::vector<VkVertexInputAttributeDescription> & attribute_descriptions)
    : device(std::move(device))
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
      {0},                                                      // blendConstants[4]
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

    const VkStencilOpState stencil_op_state{
      VK_STENCIL_OP_KEEP,                                     // failOp
      VK_STENCIL_OP_KEEP,                                     // passOp
      VK_STENCIL_OP_KEEP,                                     // depthFailOp
      VK_COMPARE_OP_ALWAYS,                                   // compareOp
      0,                                                      // compareMask
      0,                                                      // writeMask
      0,                                                      // reference
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
      0,                                               // flags
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

  std::shared_ptr<VulkanDevice> device;
  VkPipeline pipeline{ nullptr };
};
