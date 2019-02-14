#pragma once

#include <Innovator/Misc/Defines.h>
#include <vulkan/vulkan.h>
#include <functional>
#include <memory>

class VulkanTextureImage {
public:
  NO_COPY_OR_ASSIGNMENT(VulkanTextureImage)
  VulkanTextureImage() = default;
  virtual ~VulkanTextureImage() = default;

  virtual VkExtent3D extent(size_t mip_level) const = 0;
  virtual uint32_t base_level() const = 0;
  virtual uint32_t levels() const = 0;
  virtual uint32_t base_layer() const = 0;
  virtual uint32_t layers() const = 0;
  virtual size_t size() const = 0;
  virtual size_t size(size_t level) const = 0;
  virtual const unsigned char * data() const = 0;
  virtual VkFormat format() const = 0;
  virtual VkImageType image_type() const = 0;
  virtual VkImageViewType image_view_type() const = 0;
};

class VulkanImageFactory {
public:
  typedef std::function<std::shared_ptr<VulkanTextureImage>(const std::string &)> ImageFunc;

  static std::shared_ptr<VulkanTextureImage> Create(const std::string & filename)
  {
    return create_image(filename);
  }

  template <typename ImageType>
  static void Register()
  {
    create_image = [](const std::string & filename)
    {
      return std::make_shared<ImageType>(filename);
    };
  }

  static ImageFunc create_image;
};

VulkanImageFactory::ImageFunc VulkanImageFactory::create_image;