#pragma once

#include <Innovator/Core/Node.h>
#include <Innovator/Actions.h>
#include <Innovator/Core/Math/Box.h>

#include <gli/load.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include <map>
#include <utility>
#include <vector>
#include <memory>

class Separator : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(Separator)
  Separator() = default;
  virtual ~Separator() = default;

  explicit Separator(std::vector<std::shared_ptr<Node>> children)
    : Group(std::move(children)) {}

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    State s = allocator->state;
    Group::doAlloc(allocator);
    allocator->state = s;
  }

  void doStage(MemoryStager * stager) override
  {
    State s = stager->state;
    Group::doStage(stager);
    stager->state = s;
  }

  void doRecord(SceneManager * action) override
  {
    State s = action->state;
    Group::doRecord(action);
    action->state = s;
  }

  void doRender(SceneRenderer * renderer) override
  {
    RenderState s = renderer->state;
    Group::doRender(renderer);
    renderer->state = s;
  }
};

class Camera : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Camera)
  virtual ~Camera() = default;
  explicit Camera()
    : farplane(1000.0f),
      nearplane(0.1f),
      aspectratio(4.0f / 3.0f),
      fieldofview(0.7f),
      focaldistance(1.0f),
      position(glm::vec3(0, 0, 1)),
      orientation(glm::mat3(1.0))
  {}

  void zoom(float dy)
  {
    const auto focalpoint = this->position - this->orientation[2] * this->focaldistance;
    this->position += this->orientation[2] * dy;
    this->focaldistance = glm::length(this->position - focalpoint);
  }

  void pan(const glm::vec2 & dx)
  {
    this->position += this->orientation[0] * dx.x + this->orientation[1] * -dx.y;
  }

  void orbit(const glm::vec2 & dx)
  {
    const auto focalpoint = this->position - this->orientation[2] * this->focaldistance;

    auto rot = glm::mat4(this->orientation);
    rot = glm::rotate(rot, dx.y, glm::vec3(1, 0, 0));
    rot = glm::rotate(rot, dx.x, glm::vec3(0, 1, 0));
    const auto look = glm::mat3(rot) * glm::vec3(0, 0, this->focaldistance);

    this->position = focalpoint + look;
    this->lookAt(focalpoint);
  }

  void lookAt(const glm::vec3 & focalpoint)
  {
    this->orientation[2] = glm::normalize(this->position - focalpoint);
    this->orientation[0] = glm::normalize(glm::cross(this->orientation[1], this->orientation[2]));
    this->orientation[1] = glm::normalize(glm::cross(this->orientation[2], this->orientation[0]));
    this->focaldistance = glm::length(this->position - focalpoint);
  }

  void view(const box3 & box)
  {
    const auto focalpoint = box.center();
    this->focaldistance = glm::length(box.span());

    this->position = focalpoint + this->orientation[2] * this->focaldistance;
    this->lookAt(focalpoint);
  }

private:
  void doRender(SceneRenderer * renderer) override
  {
    this->aspectratio = renderer->state.viewport.width / renderer->state.viewport.height;
    renderer->state.ViewMatrix = glm::transpose(glm::mat4(this->orientation));
    renderer->state.ViewMatrix = glm::translate(renderer->state.ViewMatrix, -this->position);
    renderer->state.ProjMatrix = glm::perspective(this->fieldofview, this->aspectratio, this->nearplane, this->farplane);
  }

  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;
  float focaldistance;
  glm::vec3 position;
  glm::mat3 orientation;
};

class Transform : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Transform)
  Transform() = default;
  virtual ~Transform() = default;

  explicit Transform(const glm::vec3 & translation, 
                     const glm::vec3 & scalefactor)
    : translation(translation), 
      scaleFactor(scalefactor) {}

private:
  void doRender(SceneRenderer * renderer) override
  {
    glm::mat4 matrix(1.0);
    matrix = glm::translate(matrix, this->translation);
    matrix = glm::scale(matrix, this->scaleFactor);
    renderer->state.ModelMatrix *= matrix;
  }

  glm::vec3 translation;
  glm::vec3 scaleFactor;
};

template <typename T>
class BufferData : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(BufferData)
  BufferData() = default;
  virtual ~BufferData() = default;

  explicit BufferData(std::vector<T> values) :
    values(std::move(values))
  {}

  explicit BufferData(size_t num) : 
    num(num)
  {
    this->values.reserve(num);
  }

  std::vector<T> values;

private:
  VulkanBufferDataDescription get_buffer_data_description()
  {
    const size_t num = this->values.empty() ?
      this->num : this->values.size();

    return {
      sizeof(T) * num,                  // size
      0,                                // offset
      num,                              // count
      sizeof(T),                        // elem_size
      0,                                // usage_flags
      0,                                // memory_property_flags
      this->values.data(),              // data
      nullptr,                          // buffer
    };
  }

  void doAlloc(MemoryAllocator * allocator) override
  {
    allocator->state.bufferdata = this->get_buffer_data_description();
  }

  void doStage(MemoryStager * stager) override
  {
    stager->state.bufferdata = this->get_buffer_data_description();
  }

  void doRecord(SceneManager * action) override
  {
    action->state.bufferdata = this->get_buffer_data_description();
  }

  size_t num{ 0 };
};

class ImageData : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(ImageData)
  ImageData() = delete;
  virtual ~ImageData() = default;

  explicit ImageData(const std::string & filename) : 
    ImageData(gli::load(filename)) {}

  explicit ImageData(const gli::texture & texture) : 
    texture(texture) {}

private:
  void updateState(State & state)
  {
    state.imagedata = {
      0,                      // usage_flags
      0,                      // memory_property_flags
      &this->texture,         // texture
      nullptr,                // image
    };

    state.bufferdata.size = this->texture.size();
    state.bufferdata.data = this->texture.data();
  }

  void doAlloc(MemoryAllocator * allocator) override
  {
    this->updateState(allocator->state);
  }

  void doStage(MemoryStager * stager) override
  {
    this->updateState(stager->state);
  }

  void doRecord(SceneManager * action) override
  {
    this->doAction(action);
  }

  void doAction(SceneManager * action)
  {
    this->updateState(action->state);
  }

  gli::texture texture;
};

class CpuMemoryBuffer : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(CpuMemoryBuffer)
  CpuMemoryBuffer() = delete;
  virtual ~CpuMemoryBuffer() = default;

  explicit CpuMemoryBuffer(VkBufferUsageFlags buffer_usage_flags) :
    buffer_usage_flags(buffer_usage_flags)
  {}

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    this->buffer = std::make_shared<BufferObject>(
      allocator->state.bufferdata.size,
      0,
      this->buffer_usage_flags,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    allocator->bufferobjects.push_back(this->buffer);
  }

  void doStage(MemoryStager * stager) override
  {
    stager->state.bufferdata.offset = this->buffer->offset;
    stager->state.bufferdata.buffer = this->buffer->buffer->buffer;

    this->buffer->memcpy(stager->state.bufferdata.data);
  }

  void doRecord(SceneManager * action) override
  {
    action->state.bufferdata.buffer = this->buffer->buffer->buffer;
    action->state.bufferdata.offset = this->buffer->offset;
    action->state.bufferdata.usage_flags = this->buffer->usage;
    action->state.bufferdata.memory_property_flags = this->buffer->memory_property_flags;

    action->state.bufferdata_descriptions.push_back(action->state.bufferdata);
  }

  VkBufferUsageFlags buffer_usage_flags;
  std::shared_ptr<BufferObject> buffer{ nullptr };
};

class GpuMemoryBuffer : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(GpuMemoryBuffer)
  GpuMemoryBuffer() = delete;
  virtual ~GpuMemoryBuffer() = default;

  explicit GpuMemoryBuffer(VkBufferUsageFlags buffer_usage_flags) : 
    buffer_usage_flags(buffer_usage_flags)
  {}

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    this->buffer = std::make_shared<BufferObject>(
      allocator->state.bufferdata.size,
      0,
      this->buffer_usage_flags,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    allocator->bufferobjects.push_back(this->buffer);
  }

  void doStage(MemoryStager * stager) override
  {
    std::vector<VkBufferCopy> regions = { {
        stager->state.bufferdata.offset,  // srcOffset
        this->buffer->offset,             // dstOffset
        stager->state.bufferdata.size,    // size
    } };

    vkCmdCopyBuffer(stager->command->buffer(),
                    stager->state.bufferdata.buffer,
                    this->buffer->buffer->buffer,
                    static_cast<uint32_t>(regions.size()),
                    regions.data());
  }

  void doRecord(SceneManager * action) override
  {
    action->state.bufferdata.buffer = this->buffer->buffer->buffer;
    action->state.bufferdata.offset = this->buffer->offset;
    action->state.bufferdata.usage_flags = this->buffer->usage;
    action->state.bufferdata.memory_property_flags = this->buffer->memory_property_flags;

    action->state.bufferdata_descriptions.push_back(action->state.bufferdata);
  }

  VkBufferUsageFlags buffer_usage_flags{ 0 };
  std::shared_ptr<BufferObject> buffer{ nullptr };
};

class TransformBuffer : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(TransformBuffer)
  TransformBuffer() = default;
  virtual ~TransformBuffer() = default;

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    this->buffer = std::make_shared<BufferObject>(
      sizeof(glm::mat4) * 2,
      0,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    allocator->bufferobjects.push_back(this->buffer);
  }

  void doRecord(SceneManager * action) override
  {
    action->state.bufferdata.buffer = this->buffer->buffer->buffer;
    action->state.bufferdata.offset = this->buffer->offset;
    action->state.bufferdata.usage_flags = this->buffer->usage;
    action->state.bufferdata.memory_property_flags = this->buffer->memory_property_flags;

    action->state.bufferdata_descriptions.push_back(action->state.bufferdata);
  }

  void doRender(SceneRenderer * renderer) override
  {
    glm::mat4 data[2] = {
      renderer->state.ViewMatrix * renderer->state.ModelMatrix,
      renderer->state.ProjMatrix
    };

    this->buffer->memcpy(data);
  }

  std::shared_ptr<BufferObject> buffer{ nullptr };
};

class IndexBufferDescription : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(IndexBufferDescription)
  IndexBufferDescription() = delete;
  virtual ~IndexBufferDescription() = default;

  explicit IndexBufferDescription(VkIndexType type) : 
    type(type)
  {}

private:
  void doRecord(SceneManager * action) override
  {
    action->state.indices.push_back({
      this->type,                                             // type
      action->state.bufferdata.buffer,                        // buffer
      static_cast<uint32_t>(action->state.bufferdata.count),  // count
    });
  }

  VkIndexType type;
};

class BufferDescription : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(BufferDescription)
  BufferDescription() = default;
  virtual ~BufferDescription() = default;

private:
  void doRecord(SceneManager * action) override
  {
    action->state.buffer_descriptions.push_back({
      action->state.layout_binding,                     // layout
      action->state.bufferdata.buffer,                  // buffer 
      action->state.bufferdata.size,                    // size 
      action->state.bufferdata.offset,                  // offset 
    });
  }
};

class VertexAttributeLayout : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(VertexAttributeLayout)
  VertexAttributeLayout() = delete;
  virtual ~VertexAttributeLayout() = default;

  explicit VertexAttributeLayout(uint32_t location,
                                 uint32_t binding,
                                 VkFormat format,
                                 uint32_t offset,
                                 uint32_t stride,
                                 VkVertexInputRate inputrate) : 
    location(location),
    binding(binding),
    format(format),
    offset(offset),
    stride(stride),
    inputrate(inputrate)
  {}

private:
  void doRecord(SceneManager * action) override
  {
    action->state.attribute_descriptions.push_back({
      this->location,
      this->binding,
      this->format,
      this->offset,
      static_cast<uint32_t>(action->state.bufferdata.elem_size * this->stride),
      this->inputrate,
      action->state.bufferdata.buffer,
    });
  }

  uint32_t location;
  uint32_t binding;
  VkFormat format;
  uint32_t offset;
  uint32_t stride;
  VkVertexInputRate inputrate;
};

class LayoutBinding : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(LayoutBinding)
  LayoutBinding() = delete;
  virtual ~LayoutBinding() = default;

  LayoutBinding(uint32_t binding, 
                VkDescriptorType type, 
                VkShaderStageFlags stage) : 
    binding(binding), 
    type(type), 
    stage(stage) 
  {}

private:
  void doRecord(SceneManager * action) override
  {
    action->state.layout_binding = {
      this->binding,
      this->type,
      this->stage,
    };
  }

  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
};

class Shader : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Shader)
  Shader() = delete;
  virtual ~Shader() = default;

  Shader(std::string filename, VkShaderStageFlagBits stage) : 
    stage(stage)
  {
    std::ifstream input(filename, std::ios::binary);
    this->code = std::vector<char>((std::istreambuf_iterator<char>(input)), 
                                   (std::istreambuf_iterator<char>()));
  }

private:
  void doRecord(SceneManager * action) override
  {
    this->shader = std::make_unique<VulkanShaderModule>(action->device, this->code);

    action->state.shaders.push_back({
      this->stage,                                  // stage
      this->shader->module,                         // module
    });
  }

  std::vector<char> code;
  VkShaderStageFlagBits stage;
  std::unique_ptr<VulkanShaderModule> shader;
};

class Sampler : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Sampler)
  Sampler() = default;
  virtual ~Sampler() = default;

private:
  void doRecord(SceneManager * action) override
  {
    this->sampler = std::make_unique<VulkanSampler>(
      action->device,
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

    action->state.texture_description.sampler = this->sampler->sampler;
  }

  std::unique_ptr<VulkanSampler> sampler;
};

class Image : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Image)
  Image() = default;
  virtual ~Image() = default;

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    gli::texture * texture = allocator->state.imagedata.texture;

    VkExtent3D extent = {
      static_cast<uint32_t>(texture->extent().x),
      static_cast<uint32_t>(texture->extent().y),
      static_cast<uint32_t>(texture->extent().z)
    };

    std::shared_ptr<VulkanImage> image = std::make_shared<VulkanImage>(
      allocator->device,
      0,
      vulkan_image_type[texture->target()],
      vulkan_format[texture->format()],
      extent,
      static_cast<uint32_t>(texture->levels()),
      static_cast<uint32_t>(texture->layers()),
      VK_SAMPLE_COUNT_1_BIT,
      VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
      this->usage_flags,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>(),
      VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED);

    this->image = std::make_shared<ImageObject>(
      image,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    allocator->imageobjects.push_back(this->image);

    allocator->state.imagedata.image = this->image->image->image;
  }

  void doStage(MemoryStager * stager) override
  {
    VkComponentMapping component_mapping{
      VK_COMPONENT_SWIZZLE_R,         // r
      VK_COMPONENT_SWIZZLE_G,         // g
      VK_COMPONENT_SWIZZLE_B,         // b
      VK_COMPONENT_SWIZZLE_A          // a
    };

    const auto texture = stager->state.imagedata.texture;

    VkImageSubresourceRange subresource_range = {
      VK_IMAGE_ASPECT_COLOR_BIT,                            // aspectMask 
      static_cast<uint32_t>(texture->base_level()),         // baseMipLevel 
      static_cast<uint32_t>(texture->levels()),             // levelCount 
      static_cast<uint32_t>(texture->base_layer()),         // baseArrayLayer 
      static_cast<uint32_t>(texture->layers())              // layerCount 
    };

    this->view = std::make_unique<VulkanImageView>(
      this->image->image,
      vulkan_format[texture->format()],
      vulkan_image_view_type[texture->target()],
      component_mapping,
      subresource_range);

    VkImageMemoryBarrier memory_barrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                // sType
      nullptr,                                               // pNext
      0,                                                     // srcAccessMask
      VK_ACCESS_TRANSFER_WRITE_BIT,                          // dstAccessMask
      VK_IMAGE_LAYOUT_UNDEFINED,                             // oldLayout
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                  // newLayout
      stager->device->default_queue_index,                   // srcQueueFamilyIndex
      stager->device->default_queue_index,                   // dstQueueFamilyIndex
      this->image->image->image,                             // image
      subresource_range,                                     // subresourceRange
    };

    vkCmdPipelineBarrier(stager->command->buffer(),
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr, 1,
                         &memory_barrier);

    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < texture->levels(); mip_level++) {
      const VkExtent3D image_extent{
        static_cast<uint32_t>(texture->extent(mip_level).x),
        static_cast<uint32_t>(texture->extent(mip_level).y),
        static_cast<uint32_t>(texture->extent(mip_level).z)
      };

      const VkOffset3D image_offset = {
        0, 0, 0
      };

      const VkImageSubresourceLayers subresource_layers{
        subresource_range.aspectMask,               // aspectMask
        mip_level,                                  // mipLevel
        subresource_range.baseArrayLayer,           // baseArrayLayer
        subresource_range.layerCount                // layerCount
      };

      VkBufferImageCopy buffer_image_copy{
        buffer_offset,                             // bufferOffset 
        0,                                         // bufferRowLength
        0,                                         // bufferImageHeight
        subresource_layers,                        // imageSubresource
        image_offset,                              // imageOffset
        image_extent                               // imageExtent
      };

      vkCmdCopyBufferToImage(stager->command->buffer(),
                             stager->state.bufferdata.buffer,
                             this->image->image->image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             1,
                             &buffer_image_copy);

      buffer_offset += texture->size(mip_level);
    }
  }

  void doRecord(SceneManager * action) override
  {
    action->state.imagedata.usage_flags = this->usage_flags;
    action->state.imagedata.memory_property_flags = this->image->memory_property_flags;
    action->state.imagedata.image = this->image->image->image;

    action->state.texture_description.layout = action->state.layout_binding;
    action->state.texture_description.view = this->view->view;

    action->state.textures.push_back(action->state.texture_description);
  }

  std::shared_ptr<ImageObject> image;
  std::unique_ptr<VulkanImageView> view;
  VkImageUsageFlags usage_flags{ VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT };

  inline static std::map<gli::format, VkFormat> vulkan_format{
    { gli::format::FORMAT_R8_UNORM_PACK8, VkFormat::VK_FORMAT_R8_UNORM },
    { gli::format::FORMAT_RGBA_DXT5_UNORM_BLOCK16, VkFormat::VK_FORMAT_BC3_UNORM_BLOCK },
  };

  inline static std::map<gli::target, VkImageViewType> vulkan_image_view_type{
    { gli::target::TARGET_2D, VkImageViewType::VK_IMAGE_VIEW_TYPE_2D },
    { gli::target::TARGET_3D, VkImageViewType::VK_IMAGE_VIEW_TYPE_3D },
  };

  inline static std::map<gli::target, VkImageType> vulkan_image_type{
    { gli::target::TARGET_2D, VkImageType::VK_IMAGE_TYPE_2D },
    { gli::target::TARGET_3D, VkImageType::VK_IMAGE_TYPE_3D },
  };
};

class Texture : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(Texture)
  Texture() = delete;
  virtual ~Texture() = default;

  explicit Texture(const std::string & filename)
  {
    this->children = {
      std::make_shared<ImageData>(gli::load(filename)),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<Sampler>(),
      std::make_shared<Image>(),
    };
  }
};

class CullMode : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(CullMode)
  CullMode() = delete;
  virtual ~CullMode() = default;

  explicit CullMode(VkCullModeFlags cullmode)
    : cullmode(cullmode) {}

private:
  void doRecord(SceneManager * action) override
  {
    action->state.rasterizationstate.cullMode = this->cullmode;
  }

  VkCullModeFlags cullmode;
};

class ComputeCommand : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(ComputeCommand)
  ComputeCommand() = delete;
  virtual ~ComputeCommand() = default;

  explicit ComputeCommand(uint32_t group_count_x, 
                          uint32_t group_count_y, 
                          uint32_t group_count_z) : 
    group_count_x(group_count_x), 
    group_count_y(group_count_y), 
    group_count_z(group_count_z) 
  {}

private:
  void doRecord(SceneManager * action) override
  {
    this->pipeline = std::make_unique<ComputePipelineObject>(
      action->device,
      action->state.shaders[0],
      action->state.buffer_descriptions,
      action->state.textures,
      action->pipelinecache);

    this->pipeline->bind(action->command->buffer());

    vkCmdDispatch(action->command->buffer(),
                  this->group_count_x,
                  this->group_count_y,
                  this->group_count_z);
  }

  uint32_t group_count_x;
  uint32_t group_count_y;
  uint32_t group_count_z;

  std::unique_ptr<ComputePipelineObject> pipeline;
};

class DrawCommand : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(DrawCommand)
  DrawCommand() = delete;
  virtual ~DrawCommand() = default;

  explicit DrawCommand(VkPrimitiveTopology topology, 
                       uint32_t count) : 
    count(count),
    topology(topology)
  {}

private:
  void doRecord(SceneManager * action) override
  {
    this->pipeline = std::make_unique<GraphicsPipelineObject>(
      action->device,
      action->state.attribute_descriptions,
      action->state.shaders,
      action->state.buffer_descriptions,
      action->state.textures,
      action->state.rasterizationstate,
      action->renderpass,
      action->pipelinecache,
      this->topology);

    this->pipeline->bind(action->command->buffer());

    for (const auto& attribute : action->state.attribute_descriptions) {
      VkDeviceSize offsets[1] = { 0 };
      vkCmdBindVertexBuffers(action->command->buffer(), 0, 1, &attribute.buffer, &offsets[0]);
    }
    vkCmdSetViewport(action->command->buffer(), 0, 1, &action->viewport);
    vkCmdSetScissor(action->command->buffer(), 0, 1, &action->scissor);

    if (!action->state.indices.empty()) {
      for (const auto& indexbuffer : action->state.indices) {
        vkCmdBindIndexBuffer(action->command->buffer(), indexbuffer.buffer, 0, indexbuffer.type);
        vkCmdDrawIndexed(action->command->buffer(), indexbuffer.count, 1, 0, 0, 1);
      }
    }
    else {
      vkCmdDraw(action->command->buffer(), this->count, 1, 0, 0);
    }
  }

  uint32_t count;
  VkPrimitiveTopology topology;
  std::unique_ptr<GraphicsPipelineObject> pipeline;
};

class Box : public Separator {
public:
  NO_COPY_OR_ASSIGNMENT(Box)
  virtual ~Box() = default;

  Box()
  {
    //     5------7
    //    /|     /|
    //   / |    / |
    //  1------3  |
    //  |  4---|--6
    //  z x    | /
    //  |/     |/
    //  0--y---2

    std::vector<uint32_t> indices {
      0, 1, 3, 3, 2, 0, // -x
      4, 6, 7, 7, 5, 4, // +x
      0, 4, 5, 5, 1, 0, // -y
      6, 2, 3, 3, 7, 6, // +y
      0, 2, 6, 6, 4, 0, // -z
      1, 5, 7, 7, 3, 1, // +z
    };

    //std::vector<uint32_t> indices{
    //  1,  4,  0,
    //  4,  9,  0,
    //  4,  5,  9,
    //  8,  5,  4,
    //  1,  8,  4,
    //  1, 10,  8,
    //  10,  3, 8,
    //  8,  3,  5,
    //  3,  2,  5,
    //  3,  7,  2,
    //  3, 10,  7,
    //  10,  6, 7,
    //  6, 11,  7,
    //  6,  0, 11,
    //  6,  1,  0,
    //  10,  1, 6,
    //  11,  0, 9,
    //  2, 11,  9,
    //  5,  2,  9,
    //  11,  2, 7
    //};


    std::vector<float> vertices {
      0, 0, 0, // 0
      0, 0, 1, // 1
      0, 1, 0, // 2
      0, 1, 1, // 3
      1, 0, 0, // 4
      1, 0, 1, // 5
      1, 1, 0, // 6
      1, 1, 1, // 7
    };

    //const float t = float(1 + std::pow(5, 0.5)) / 2;  // golden ratio
    //std::vector<float> vertices{
    //  -1,  0,  t,
    //   1,  0,  t,
    //  -1,  0, -t,
    //   1,  0, -t,
    //   0,  t,  1,
    //   0,  t, -1,
    //   0, -t,  1,
    //   0, -t, -1,
    //   t,  1,  0,
    //  -t,  1,  0,
    //   t, -1,  0,
    //  -t, -1,  0
    //};

    this->children = {
      std::make_shared<BufferData<uint32_t>>(indices),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      std::make_shared<IndexBufferDescription>(VK_INDEX_TYPE_UINT32),
      std::make_shared<BufferData<float>>(vertices),
      std::make_shared<CpuMemoryBuffer>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT),
      std::make_shared<GpuMemoryBuffer>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
      std::make_shared<VertexAttributeLayout>(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0, 3, VK_VERTEX_INPUT_RATE_VERTEX),
      std::make_shared<TransformBuffer>(),
      std::make_shared<LayoutBinding>(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS),
      std::make_shared<BufferDescription>(),
      std::make_shared<DrawCommand>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 36)
    };
  }
};
