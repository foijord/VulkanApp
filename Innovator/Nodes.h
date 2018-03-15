#pragma once

#include <Innovator/Core/Node.h>
#include <Innovator/Actions.h>
#include <Innovator/Group.h>

#include <gli/load.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include <map>
#include <utility>
#include <vector>
#include <memory>
#include <iostream>

class Separator : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(Separator);
  Separator() = default;
  virtual ~Separator() = default;

  explicit Separator(const std::vector<std::shared_ptr<Node>> & children)
    : Group(children) {}

private:
  void do_traverse(RenderAction * action) override
  {
    StateScope scope(action);
    this->traverse_children(action);
  }

  void do_traverse(BoundingBoxAction * action) override
  {
    StateScope scope(action);
    this->traverse_children(action);
  }

  void do_traverse(HandleEventAction * action) override
  {
    StateScope scope(action);
    this->traverse_children(action);
  }
};

class Camera : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Camera);
  virtual ~Camera() = default;
  explicit Camera()
    : farPlane(1000.0f),
      nearPlane(0.1f),
      aspectRatio(4.0f / 3.0f),
      fieldOfView(0.7f),
      focalDistance(1.0f),
      position(glm::vec3(0, 0, 1)),
      orientation(glm::mat3(1.0))
  {}

  void zoom(float dy)
  {
    vec3 focalpoint = this->position - this->orientation[2] * this->focalDistance;
    this->position += this->orientation[2] * dy;
    this->focalDistance = glm::length(this->position - focalpoint);
  }

  void pan(const glm::vec2 & dx)
  {
    this->position += this->orientation[0] * dx.x + this->orientation[1] * -dx.y;
  }

  void orbit(const glm::vec2 & dx)
  {
    glm::vec3 focalpoint = this->position - this->orientation[2] * this->focalDistance;

    glm::mat4 rot = glm::mat4(this->orientation);
    rot = glm::rotate(rot, dx.y, glm::vec3(1, 0, 0));
    rot = glm::rotate(rot, dx.x, glm::vec3(0, 1, 0));
    glm::vec3 look = glm::mat3(rot) * glm::vec3(0, 0, this->focalDistance);

    this->position = focalpoint + look;
    this->lookAt(focalpoint);
  }

  void lookAt(const glm::vec3 & focalpoint)
  {
    this->orientation[2] = glm::normalize(this->position - focalpoint);
    this->orientation[0] = glm::normalize(glm::cross(this->orientation[1], this->orientation[2]));
    this->orientation[1] = glm::normalize(glm::cross(this->orientation[2], this->orientation[0]));
    this->focalDistance = glm::length(this->position - focalpoint);
  }

  void viewAll(const std::shared_ptr<Group> & root)
  {
    BoundingBoxAction action;
    root->traverse(&action);
    glm::vec3 focalpoint = action.bounding_box.center();
    this->focalDistance = length(action.bounding_box.span());

    this->position = focalpoint + this->orientation[2] * this->focalDistance;
    this->lookAt(focalpoint);
  }

private:
  void do_traverse(RenderAction * action) override
  {
    this->aspectRatio = action->viewport.width / action->viewport.height;
    action->state.ViewMatrix = glm::transpose(glm::mat4(this->orientation));
    action->state.ViewMatrix = glm::translate(action->state.ViewMatrix, -this->position);
    action->state.ProjMatrix = glm::perspective(this->fieldOfView, this->aspectRatio, this->nearPlane, this->farPlane);
  }

  float farPlane;
  float nearPlane;
  float aspectRatio;
  float fieldOfView;
  float focalDistance;
  glm::vec3 position;
  glm::mat3 orientation;
};

class Transform : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Transform);
  Transform() = default;
  virtual ~Transform() = default;

  explicit Transform(const glm::vec3 & translation = glm::vec3(0), const glm::vec3 & scalefactor = glm::vec3(1))
    : translation(translation), scaleFactor(scalefactor) {}

private:
  void do_traverse(BoundingBoxAction * action) override
  {
    this->do_action(action);
  }

  void do_traverse(RenderAction * action) override
  {
    this->do_action(action);
  }

  void do_action(Action * action) const
  {
    mat4 matrix(1.0);
    matrix = glm::translate(matrix, this->translation);
    matrix = glm::scale(matrix, this->scaleFactor);
    action->state.ModelMatrix *= matrix;
  }

  glm::vec3 translation;
  glm::vec3 scaleFactor;
};

template <typename T>
class BufferData : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(BufferData);
  BufferData() = default;
  virtual ~BufferData() = default;

  std::vector<T> values;

private:
  void do_traverse(RenderAction * action) override
  {
    const VulkanBufferDataDescription bufferdata{
      sizeof(T) * this->values.size(),
      this->values.size(),
      sizeof(T),
      this->values.data(),
    };

    action->state.bufferdata = bufferdata;
  }
};

class DeviceMemoryBuffer : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(DeviceMemoryBuffer);
  DeviceMemoryBuffer() = delete;
  virtual ~DeviceMemoryBuffer() = default;

  explicit DeviceMemoryBuffer(VkBufferUsageFlags usage)
    : usage(usage)
  {}

protected:
  void init_buffers(RenderAction * action)
  {
    if (!this->gpu_buffer) {
      this->cpu_buffer = std::make_unique<VulkanBufferObject>(
        action->device,
        action->state.bufferdata.size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

      this->gpu_buffer = std::make_unique<VulkanBufferObject>(
        action->device,
        action->state.bufferdata.size,
        this->usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      this->copy_data(action);
    }
  }

  void copy_data(RenderAction * action)
  {
    this->cpu_buffer->memory->memcpy(
      action->state.bufferdata.data,
      action->state.bufferdata.size);

    std::vector<VkBufferCopy> regions = { {
        0,                              // srcOffset
        0,                              // dstOffset 
        action->state.bufferdata.size,  // size 
      } };

    vkCmdCopyBuffer(action->command->buffer(),
      this->cpu_buffer->buffer->buffer,
      this->gpu_buffer->buffer->buffer,
      static_cast<uint32_t>(regions.size()),
      regions.data());
  }

  VkBufferUsageFlags usage;
  std::unique_ptr<VulkanBufferObject> cpu_buffer;
  std::unique_ptr<VulkanBufferObject> gpu_buffer;
};

class IndexBufferDescription : public DeviceMemoryBuffer {
public:
  NO_COPY_OR_ASSIGNMENT(IndexBufferDescription);
  virtual ~IndexBufferDescription() = default;
  explicit IndexBufferDescription(VkIndexType index_type)
    : DeviceMemoryBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
      index_type(index_type)
  {}

private:
  void do_traverse(RenderAction * action) override
  {
    this->init_buffers(action);

    action->state.indices.push_back({
      this->index_type,                                       // type
      this->gpu_buffer->buffer->buffer,                       // buffer
      static_cast<uint32_t>(action->state.bufferdata.count),  // count
    });
  }

  VkIndexType index_type;
};

class BufferDescription : public DeviceMemoryBuffer {
public:
  NO_COPY_OR_ASSIGNMENT(BufferDescription);
  BufferDescription() = delete;
  virtual ~BufferDescription() = default;

  explicit BufferDescription(VkBufferUsageFlagBits usage)
    : DeviceMemoryBuffer(usage)
  {}

protected:
  void update_state(RenderAction * action)
  {
    action->state.buffer_descriptions.push_back({
      action->state.layout_binding.binding,             // binding 
      action->state.layout_binding.type,                // type
      action->state.layout_binding.stage,               // stage 
      this->gpu_buffer->buffer->buffer,                 // buffer 
      action->state.bufferdata.size,                    // size 
      0                                                 // offset 
    });
  }

private:
  void do_traverse(RenderAction * action) override
  {
    this->init_buffers(action);
    this->copy_data(action);
    this->update_state(action);
  }
};

class UniformBuffer : public BufferDescription {
public:
  NO_COPY_OR_ASSIGNMENT(UniformBuffer);
  virtual ~UniformBuffer() = default;
  UniformBuffer() : BufferDescription(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {}
};

class VertexAttribute : public DeviceMemoryBuffer {
public:
  NO_COPY_OR_ASSIGNMENT(VertexAttribute);
  VertexAttribute() = delete;
  virtual ~VertexAttribute() = default;

  VertexAttribute(uint32_t location,
                  uint32_t binding, 
                  VkFormat format, 
                  uint32_t offset,
                  uint32_t stride,
                  VkVertexInputRate inputrate)
    : DeviceMemoryBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
      location(location), 
      binding(binding), 
      format(format), 
      offset(offset),
      stride(stride),
      inputrate(inputrate) 
  {}

private:
  void do_traverse(RenderAction * action) override
  {
    this->init_buffers(action);

    action->state.attribute_descriptions.push_back({
      this->location,
      this->binding,
      this->format,
      this->offset,
      static_cast<uint32_t>(action->state.bufferdata.elem_size * this->stride),
      this->inputrate,
      this->gpu_buffer->buffer->buffer,
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
  NO_COPY_OR_ASSIGNMENT(LayoutBinding);
  LayoutBinding() = delete;
  virtual ~LayoutBinding() = default;

  LayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage)
    : binding(binding), type(type), stage(stage) {}

private:
  void do_traverse(RenderAction * action) override
  {
    action->state.layout_binding.type = this->type;
    action->state.layout_binding.stage = this->stage;
    action->state.layout_binding.binding = this->binding;
  }

  uint32_t binding;
  VkDescriptorType type;
  VkShaderStageFlags stage;
};

class Shader : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Shader);
  Shader() = delete;
  virtual ~Shader() = default;

  Shader(std::string filename, VkShaderStageFlagBits stage)
    : filename(std::move(filename)), stage(stage) {}

private:
  void do_traverse(RenderAction * action) override
  {
    if (!this->shader) {
      std::ifstream input(filename, std::ios::binary);
      std::vector<char> code((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
      this->shader = std::make_unique<VulkanShaderModule>(action->device, code);
    }

    const VulkanShaderModuleDescription shader {
      this->stage,          // stage
      this->shader->module, // module
    };
    action->state.shaders.push_back(shader);
  }

  std::string filename;
  VkShaderStageFlagBits stage;
  std::unique_ptr<VulkanShaderModule> shader;
};

class Sampler : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Sampler);
  Sampler() = default;
  virtual ~Sampler() = default;

private:
  void do_traverse(RenderAction * action) override
  {
    if (!this->sampler) {
      this->sampler = std::make_unique<VulkanSampler>(
        action->device,
        VkFilter::VK_FILTER_LINEAR,
        VkFilter::VK_FILTER_LINEAR,
        VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR,
        VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        0.f,
        VK_FALSE,
        1.f,
        VK_FALSE,
        VkCompareOp::VK_COMPARE_OP_NEVER,
        0.f,
        0.f,
        VkBorderColor::VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        VK_FALSE);
    }

    action->state.texture_description.sampler = this->sampler->sampler;
  }
  std::unique_ptr<VulkanSampler> sampler;
};

class Texture : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Texture);
  Texture() = delete;
  virtual ~Texture() = default;

  explicit Texture(const std::string & filename)
    : Texture(gli::load(filename)) {}

  explicit Texture(const gli::texture & texture)
    : texture(texture) {}

private:
  void do_traverse(RenderAction * action) override
  {
    if (!this->image) {
      VkComponentMapping component_mapping = {
        VK_COMPONENT_SWIZZLE_R, // r
        VK_COMPONENT_SWIZZLE_G, // g
        VK_COMPONENT_SWIZZLE_B, // b
        VK_COMPONENT_SWIZZLE_A  // a
      };

      VkImageSubresourceRange subresource_range = {
        VK_IMAGE_ASPECT_COLOR_BIT,                   // aspectMask 
        static_cast<uint32_t>(texture.base_level()), // baseMipLevel 
        static_cast<uint32_t>(texture.levels()),     // levelCount 
        static_cast<uint32_t>(texture.base_layer()), // baseArrayLayer 
        static_cast<uint32_t>(texture.layers())      // layerCount 
      };

      VkExtent3D extent = {
        static_cast<uint32_t>(this->texture.extent().x),
        static_cast<uint32_t>(this->texture.extent().y),
        static_cast<uint32_t>(this->texture.extent().z)
      };

      std::map<gli::target, VkImageType> vulkan_image_type {
        { gli::target::TARGET_2D, VkImageType::VK_IMAGE_TYPE_2D },
        { gli::target::TARGET_3D, VkImageType::VK_IMAGE_TYPE_3D },
      };

      std::map<gli::target, VkImageViewType> vulkan_image_view_type {
        { gli::target::TARGET_2D, VkImageViewType::VK_IMAGE_VIEW_TYPE_2D },
        { gli::target::TARGET_3D, VkImageViewType::VK_IMAGE_VIEW_TYPE_3D },
      };

      std::map<gli::format, VkFormat> vulkan_format{
        { gli::format::FORMAT_R8_UNORM_PACK8, VkFormat::VK_FORMAT_R8_UNORM },
        { gli::format::FORMAT_RGBA_DXT5_UNORM_BLOCK16, VkFormat::VK_FORMAT_BC3_UNORM_BLOCK },
      };

      this->image = std::make_unique<ImageObject>(
        action->device,
        vulkan_format[this->texture.format()],
        extent,
        VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
        VkImageTiling::VK_IMAGE_TILING_OPTIMAL,
        vulkan_image_type[this->texture.target()],
        vulkan_image_view_type[this->texture.target()],
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        subresource_range,
        component_mapping);

      this->image->setData(action->command->buffer(), this->texture);
    }
    action->state.texture_description.type = action->state.layout_binding.type;
    action->state.texture_description.stage = action->state.layout_binding.stage;
    action->state.texture_description.binding = action->state.layout_binding.binding;
    action->state.texture_description.view = this->image->view->view;

    action->state.textures.push_back(action->state.texture_description);
  }
  
  gli::texture texture;
  std::unique_ptr<ImageObject> image;
};

class CullMode : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(CullMode);
  CullMode() = delete;
  virtual ~CullMode() = default;

  explicit CullMode(VkCullModeFlags cullmode)
    : cullmode(cullmode) {}

private:
  void do_traverse(RenderAction * action) override
  {
    action->state.rasterizationstate.cullMode = this->cullmode;
  }

  VkCullModeFlags cullmode;
};

class RasterizationState : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(RasterizationState);
  virtual ~RasterizationState() = default;

  explicit RasterizationState(const VkPipelineRasterizationStateCreateInfo & rasterizationstate = State::defaultRasterizationState())
    : state(rasterizationstate)
  {}

private:
  void do_traverse(RenderAction * action) override
  {
    action->state.rasterizationstate = this->state;
  }

  VkPipelineRasterizationStateCreateInfo state;
};

class Command : public Node {
public:
  virtual ~Command() = default;
  std::unique_ptr<VulkanCommandBuffers> command;
};

class ComputeCommand : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(ComputeCommand);
  ComputeCommand() = delete;
  virtual ~ComputeCommand() = default;

  explicit ComputeCommand(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
    : group_count_x(group_count_x), group_count_y(group_count_y), group_count_z(group_count_z) {}

private:
  void do_traverse(RenderAction * action) override
  {
    action->state.compute_description.group_count_x = this->group_count_x;
    action->state.compute_description.group_count_y = this->group_count_y;
    action->state.compute_description.group_count_z = this->group_count_z;
    action->compute_states.push_back(action->state);
  }

  uint32_t group_count_x;
  uint32_t group_count_y;
  uint32_t group_count_z;
};

class DrawCommand : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(DrawCommand);
  DrawCommand() = delete;
  virtual ~DrawCommand() = default;

  explicit DrawCommand(VkPrimitiveTopology topology, uint32_t count = 0)
    : count(count),
      topology(topology),
      transform_data(std::make_shared<BufferData<glm::mat4>>()),
      transform(std::make_shared<UniformBuffer>()),
      transform_layout(std::make_shared<LayoutBinding>(0, 
                                                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
                                                       VK_SHADER_STAGE_ALL_GRAPHICS))
  {}

private:
  void do_traverse(RenderAction * action) override
  {
    this->transform_data->values = {
      action->state.ViewMatrix * action->state.ModelMatrix, 
      action->state.ProjMatrix 
    };
    this->transform_layout->traverse(action);
    this->transform_data->traverse(action);
    this->transform->traverse(action);

    action->state.drawdescription.count = this->count;
    action->state.drawdescription.topology = this->topology;
    action->graphic_states.push_back(action->state);
  }

  void do_traverse(BoundingBoxAction * action) override
  {
    box3 box(vec3(0), vec3(1));
    box.transform(action->state.ModelMatrix);
    action->extendBy(box);
  }

  uint32_t count;
  VkPrimitiveTopology topology;
  std::shared_ptr<BufferData<glm::mat4>> transform_data;
  std::shared_ptr<BufferDescription> transform;
  std::shared_ptr<LayoutBinding> transform_layout;
};

class Box : public Separator {
public:
  NO_COPY_OR_ASSIGNMENT(Box);
  Box() = delete;
  virtual ~Box() = default;

  explicit Box(uint32_t binding, uint32_t location)
  {
    //     5------7
    //    /|     /|
    //   / |    / |
    //  1------3  |
    //  |  4---|--6
    //  z x    | /
    //  |/     |/
    //  0--y---2
    std::shared_ptr<BufferData<float>> vertices = std::make_shared<BufferData<float>>();
    vertices->values = {
      0, 0, 0, // 0
      0, 0, 1, // 1
      0, 1, 0, // 2
      0, 1, 1, // 3
      1, 0, 0, // 4
      1, 0, 1, // 5
      1, 1, 0, // 6
      1, 1, 1, // 7
    };

    std::shared_ptr<BufferData<uint32_t>> indices = std::make_shared<BufferData<uint32_t>>();
    indices->values = {
      0, 1, 3, 3, 2, 0, // -x
      4, 6, 7, 7, 5, 4, // +x
      0, 4, 5, 5, 1, 0, // -y
      6, 2, 3, 3, 7, 6, // +y
      0, 2, 6, 6, 4, 0, // -z
      1, 5, 7, 7, 3, 1, // +z
    };

    this->children = {
      indices,
      std::make_shared<IndexBufferDescription>(VK_INDEX_TYPE_UINT32),
      vertices,
      std::make_shared<VertexAttribute>(
        location,
        binding,
        VK_FORMAT_R32G32B32_SFLOAT,
        0,
        3,
        VK_VERTEX_INPUT_RATE_VERTEX),
      std::make_shared<DrawCommand>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 36)
    };
  }

private:
  void do_traverse(BoundingBoxAction * action) override
  {
    box3 box(vec3(0), vec3(1));
    box.transform(action->state.ModelMatrix);
    action->extendBy(box);
  }
};

class Sphere : public Separator {
public:
  NO_COPY_OR_ASSIGNMENT(Sphere);
  Sphere() = delete;
  virtual ~Sphere() = default;

  explicit Sphere(uint32_t binding, uint32_t location)
  {
    std::shared_ptr<BufferData<uint32_t>> indices = std::make_shared<BufferData<uint32_t>>();
    indices->values = {
      1,  4,  0,
      4,  9,  0,
      4,  5,  9,
      8,  5,  4,
      1,  8,  4,
      1, 10,  8,
      10,  3, 8,
      8,  3,  5,
      3,  2,  5,
      3,  7,  2,
      3, 10,  7,
      10,  6, 7,
      6, 11,  7,
      6,  0, 11,
      6,  1,  0,
      10,  1, 6,
      11,  0, 9,
      2, 11,  9,
      5,  2,  9,
      11,  2, 7
    };

    float t = float(1 + std::pow(5, 0.5)) / 2;  // golden ratio
    std::shared_ptr<BufferData<float>> vertices = std::make_shared<BufferData<float>>();

    vertices->values = {
      -1, 0, t,
       1, 0, t,
      -1, 0,-t,
       1, 0,-t,
       0, t, 1,
       0, t,-1,
       0,-t, 1,
       0,-t,-1,
       t, 1, 0,
      -t, 1, 0,
       t,-1, 0,
      -t,-1, 0
    };

    this->children = {
      indices,
      std::make_shared<IndexBufferDescription>(VK_INDEX_TYPE_UINT32),
      vertices,
      std::make_shared<VertexAttribute>(
        location,
        binding,
        VK_FORMAT_R32G32B32_SFLOAT,
        0,
        3,
        VK_VERTEX_INPUT_RATE_VERTEX),
      std::make_shared<DrawCommand>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 36)
    };
  }

  void do_traverse(BoundingBoxAction * action) override
  {
    box3 box(vec3(0), vec3(1));
    box.transform(action->state.ModelMatrix);
    action->extendBy(box);
  }
};