
#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <VulkanWindow.h>
#endif
#include <Innovator/File.h>
#include <Innovator/Misc/Factory.h>

#include <iostream>
#include <vector>


int main(int argc, char *argv[])
{
  try {
    VulkanImageFactory::Register<GliTextureImage>();

    std::vector<const char *> instance_layers{
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    std::vector<const char *> instance_extensions{
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
      VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif
    };

    auto vulkan = std::make_shared<VulkanInstance>("Innovator",
                                                    instance_layers,
                                                    instance_extensions);

#ifdef _DEBUG
    auto debugcb = std::make_unique<VulkanDebugCallback>(
      vulkan,
      VK_DEBUG_REPORT_WARNING_BIT_EXT |
      VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
      VK_DEBUG_REPORT_ERROR_BIT_EXT);
#endif

	std::vector<const char *> device_layers{
#ifdef _DEBUG
      "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    std::vector<const char *> device_extensions{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkPhysicalDeviceFeatures device_features;
    ::memset(&device_features, VK_FALSE, sizeof(VkPhysicalDeviceFeatures));

    auto device = std::make_shared<VulkanDevice>(vulkan,
                                                 device_features,
                                                 device_layers,
                                                 device_extensions);

    auto color_attachment = std::make_shared<FramebufferAttachment>(VK_FORMAT_B8G8R8A8_UNORM,
                                                                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                    VK_IMAGE_ASPECT_COLOR_BIT);

    auto depth_attachment = std::make_shared<FramebufferAttachment>(VK_FORMAT_D32_SFLOAT,
                                                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                                    VK_IMAGE_ASPECT_DEPTH_BIT);

    std::shared_ptr<Group> renderpass = std::make_shared<RenderpassObject>(
      std::vector<VkAttachmentDescription>{ {
        0,                                                    // flags
        color_attachment->format,                             // format
        VK_SAMPLE_COUNT_1_BIT,                                // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                         // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                     // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                            // initialLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL              // finalLayout
      }, {
        0,                                                    // flags
        depth_attachment->format,                             // format
        VK_SAMPLE_COUNT_1_BIT,                                // samples
        VK_ATTACHMENT_LOAD_OP_CLEAR,                          // loadOp
        VK_ATTACHMENT_STORE_OP_STORE,                         // storeOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,                      // stencilLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,                     // stencilStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                            // initialLayout
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL      // finalLayout
      } },
      std::vector<std::shared_ptr<SubpassObject>>{
        std::make_shared<SubpassObject>(
          0,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          std::vector<VkAttachmentReference>{},
          std::vector<VkAttachmentReference>{ { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL } },
          std::vector<VkAttachmentReference>{},
          VkAttachmentReference{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL },
          std::vector<uint32_t>{}
        )});


    auto framebuffer = std::make_shared<FramebufferObject>();
    framebuffer->children = {
      color_attachment,
      depth_attachment
    };

    auto camera = std::make_shared<Camera>(1000.0f, 0.1f, 4.0f / 3, 0.7f);
    camera->lookAt({ 0, 2, 4 }, { 0, 0, 0 }, { 0, 1, 0 });

    renderpass->children = {
      framebuffer,
      camera,
      eval_file("crate.scene")
    };

    VulkanWindow window(vulkan, device, color_attachment, renderpass, camera);
    return window.show();
  }
  catch (std::exception & e) {
    std::cerr << std::string("caught exception in main(): ") + typeid(e).name() << std::endl;
  }
  return 1;
}
