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

template <typename Traverser, typename State>
class StateScope {
public:
  NO_COPY_OR_ASSIGNMENT(StateScope)
  StateScope() = delete;

  explicit StateScope(Traverser * traverser) : 
    traverser(traverser),
    state(traverser->state)
  {}

  ~StateScope()
  {
    traverser->state = this->state;
  }
  
  Traverser * traverser;
  State state;
};

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
    StateScope<MemoryAllocator, MemoryState> scope(allocator);
    Group::doAlloc(allocator);
  }

  void doStage(MemoryStager * stager) override
  {
    StateScope<MemoryStager, StageState> scope(stager);
    Group::doStage(stager);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    StateScope<CommandRecorder, RecordState> scope(recorder);
    Group::doRecord(recorder);
  }

  void doRender(SceneRenderer * renderer) override
  {
    StateScope<SceneRenderer, RenderState> scope(renderer);
    Group::doRender(renderer);
  }
};

class Camera : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Camera)
  virtual ~Camera() = default;

  explicit Camera() : 
    farplane(1000.0f),
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
    this->aspectratio = renderer->viewport.width / renderer->viewport.height;
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

  std::vector<T> values;

private:
  template <typename StateType>
  void initState(StateType & state)
  {
    state.bufferdata = {
      sizeof(T),                        // stride
      sizeof(T) * this->values.size(),  // size
      this->values.data(),              // data
    };
  }

  void doAlloc(MemoryAllocator * allocator) override
  {
    this->initState(allocator->state);
  }

  void doStage(MemoryStager * stager) override
  {
    this->initState(stager->state);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    this->initState(recorder->state);
  }
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
  template <typename StateType>
  void initState(StateType & state)
  {
    state.imagedata = {
      &this->texture,                   // texture
    };

    state.bufferdata = {
      1,                                // stride
      this->texture.size(),             // size
      this->texture.data(),             // data
    };
  }

  void doAlloc(MemoryAllocator * allocator) override
  {
    this->initState(allocator->state);
  }

  void doStage(MemoryStager * stager) override
  {
    this->initState(stager->state);
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
  template <typename StateType>
  void updateState(StateType & state)
  {
    state.buffer = {
      this->buffer->buffer->buffer,      // buffer
      this->buffer->offset,              // offset
      this->buffer->range,               // range
    };
  }

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
    this->updateState(stager->state);
    this->buffer->memcpy(stager->state.bufferdata.data);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    this->updateState(recorder->state);
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
        stager->state.buffer.offset,      // srcOffset
        this->buffer->offset,             // dstOffset
        stager->state.bufferdata.size,    // size
    } };

    vkCmdCopyBuffer(stager->command->buffer(),
                    stager->state.buffer.buffer,
                    this->buffer->buffer->buffer,
                    static_cast<uint32_t>(regions.size()),
                    regions.data());
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.buffer = {
      this->buffer->buffer->buffer,      // buffer
      this->buffer->offset,              // offset
      this->buffer->range,               // range
    };
  }

  VkBufferUsageFlags buffer_usage_flags;
  std::shared_ptr<BufferObject> buffer{ nullptr };
};

class TransformBuffer : public Node {
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

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.buffer = {
      this->buffer->buffer->buffer,      // buffer
      this->buffer->offset,              // offset
      this->buffer->range,               // range
    };
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
  void doRecord(CommandRecorder * recorder) override
  {
    const auto count = recorder->state.bufferdata.size / recorder->state.bufferdata.stride;

    recorder->state.indices.push_back({
      this->type,                                               // type
      recorder->state.buffer.buffer,                            // buffer
      static_cast<uint32_t>(count),                             // count
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
  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.buffer_descriptions.push_back({
      recorder->state.layout_binding,                     // layout
      recorder->state.buffer,                             // buffer
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
  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.attribute_descriptions.push_back({
      this->location,
      this->binding,
      this->format,
      this->offset,
      static_cast<uint32_t>(recorder->state.bufferdata.stride * this->stride),
      this->inputrate,
      recorder->state.buffer.buffer,
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
  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.layout_binding = {
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
  void doRecord(CommandRecorder * recorder) override
  {
    this->shader = std::make_unique<VulkanShaderModule>(recorder->device, this->code);

    recorder->state.shaders.push_back({
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
  void doRecord(CommandRecorder * recorder) override
  {
    this->sampler = std::make_unique<VulkanSampler>(
      recorder->device,
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

    recorder->state.texture_description.image.sampler = this->sampler->sampler;
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
    const auto texture = allocator->state.imagedata.texture;

    VkExtent3D extent = {
      static_cast<uint32_t>(texture->extent().x),
      static_cast<uint32_t>(texture->extent().y),
      static_cast<uint32_t>(texture->extent().z)
    };

    auto image = std::make_shared<VulkanImage>(
      allocator->device,
      0,
      vulkan_image_type[texture->target()],
      vulkan_format[texture->format()],
      extent,
      static_cast<uint32_t>(texture->levels()),
      static_cast<uint32_t>(texture->layers()),
      VK_SAMPLE_COUNT_1_BIT,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>(),
      VK_IMAGE_LAYOUT_UNDEFINED);

    this->image = std::make_shared<ImageObject>(
      image,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    allocator->imageobjects.push_back(this->image);
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
                             stager->state.buffer.buffer,
                             this->image->image->image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             1,
                             &buffer_image_copy);

      buffer_offset += texture->size(mip_level);
    }
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.texture_description.layout = recorder->state.layout_binding;
    recorder->state.texture_description.image.imageView = this->view->view;
    recorder->state.texture_description.image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    recorder->state.textures.push_back(recorder->state.texture_description);
  }

  std::shared_ptr<ImageObject> image;
  std::unique_ptr<VulkanImageView> view;

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
  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.rasterizationstate.cullMode = this->cullmode;
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
  void doRecord(CommandRecorder * recorder) override
  {
    this->pipeline = std::make_unique<ComputePipelineObject>(
      recorder->device,
      recorder->state.shaders[0],
      recorder->state.buffer_descriptions,
      recorder->state.textures,
      recorder->pipelinecache);

    this->pipeline->bind(recorder->command->buffer());

    vkCmdDispatch(recorder->command->buffer(),
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
  void doRecord(CommandRecorder * recorder) override
  {
    this->pipeline = std::make_unique<GraphicsPipelineObject>(
      recorder->device,
      recorder->renderpass,
      recorder->pipelinecache,
      recorder->state.attribute_descriptions,
      recorder->state.shaders,
      recorder->state.buffer_descriptions,
      recorder->state.textures,
      recorder->state.rasterizationstate,
      this->topology);

    this->pipeline->bind(recorder->command->buffer());

    for (const auto& attribute : recorder->state.attribute_descriptions) {
      VkDeviceSize offsets[1] = { 0 };
      vkCmdBindVertexBuffers(recorder->command->buffer(), 0, 1, &attribute.buffer, &offsets[0]);
    }

    if (!recorder->state.indices.empty()) {
      for (const auto& indexbuffer : recorder->state.indices) {
        vkCmdBindIndexBuffer(recorder->command->buffer(), indexbuffer.buffer, 0, indexbuffer.type);
        vkCmdDrawIndexed(recorder->command->buffer(), indexbuffer.count, 1, 0, 0, 1);
      }
    }
    else {
      vkCmdDraw(recorder->command->buffer(), this->count, 1, 0, 0);
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
