#pragma once

#include <Innovator/Actions.h>
#include <Innovator/Core/Node.h>

#include <gli/load.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include <map>
#include <vector>
#include <memory>
#include <iostream>

class Group : public Node {
public:
  Group() {}
  Group(std::vector<std::shared_ptr<Node>> children)
    : children(children) {}

  virtual ~Group() {}
  virtual void traverse(RenderAction * action)
  {
    for (const std::shared_ptr<Node> & node : this->children) {
      node->traverse(action);
    }
  }

  virtual void traverse(BoundingBoxAction * action)
  {
    for (const std::shared_ptr<Node> & node : this->children) {
      node->traverse(action);
    }
  }

  virtual void traverse(HandleEventAction * action)
  {
    for (const std::shared_ptr<Node> & node : this->children) {
      node->traverse(action);
    }
  }

  std::vector<std::shared_ptr<Node>> children;
};

class Separator : public Group {
public:
  Separator() {}
  Separator(std::vector<std::shared_ptr<Node>> children)
    : Group(children) {}

  virtual ~Separator() {}
  
  static std::shared_ptr<Separator> create(std::vector<std::shared_ptr<Node>> children)
  {
    return std::make_shared<Separator>(children);
  }

  virtual void traverse(RenderAction * action)
  {
    StateScope scope(action);
    Group::traverse(action);
  }

  virtual void traverse(BoundingBoxAction * action)
  {
    StateScope scope(action);
    Group::traverse(action);
  }

  virtual void traverse(HandleEventAction * action)
  {
    StateScope scope(action);
    Group::traverse(action);
  }
};

class Camera : public Node {
public:
  Camera()
    : orientation(glm::mat3(1.0)),
      aspectRatio(4.0f / 3.0f),
      fieldOfView(0.7f),
      nearPlane(0.1f),
      farPlane(1000.0f),
      position(glm::vec3(0, 0, 1)),
      focalDistance(1.0f)
  {}

  virtual ~Camera() {}

  virtual void traverse(RenderAction * action)
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
};


class Transform : public Node {
public:
  Transform(const glm::vec3 & translation = glm::vec3(0), const glm::vec3 & scalefactor = glm::vec3(1))
    : translation(translation), scaleFactor(scalefactor) {}

  virtual ~Transform() {}

  virtual void traverse(RenderAction * action)
  {
    this->doAction(action);
  }

  virtual void traverse(BoundingBoxAction * action)
  {
    this->doAction(action);
  }

  glm::vec3 translation;
  glm::vec3 scaleFactor;

private:
  void doAction(Action * action)
  {
    mat4 matrix(1.0);
    matrix = glm::translate(matrix, this->translation);
    matrix = glm::scale(matrix, this->scaleFactor);
    action->state.ModelMatrix *= matrix;
  }
};

class IndexBuffer : public Node {
public:
  IndexBuffer() {}
  virtual ~IndexBuffer() {}

  virtual void traverse(RenderAction * action)
  {
    if (!this->gpu_buffer) {
      size_t size = sizeof(uint32_t) * this->values.size();
      this->cpu_buffer = std::make_unique<VulkanBufferObject>(action->device, 
                                                              size, 
                                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

      this->gpu_buffer = std::make_unique<VulkanBufferObject>(action->device, 
                                                              size, 
                                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      this->cpu_buffer->memory->memcpy(this->values.data(), size);

      std::vector<VkBufferCopy> regions = {{
        0,    // srcOffset
        0,    // dstOffset 
        size, // size 
      }};

      vkCmdCopyBuffer(action->command->buffer(), 
                      this->cpu_buffer->buffer->buffer, 
                      this->gpu_buffer->buffer->buffer, 
                      static_cast<uint32_t>(regions.size()),
                      regions.data());
    }
    VulkanIndexBufferDescription indices;
    indices.type = VK_INDEX_TYPE_UINT32;
    indices.buffer = this->gpu_buffer->buffer->buffer;
    indices.count = static_cast<uint32_t>(this->values.size());
    action->state.indices.push_back(indices);
  }
  
  std::vector<uint32_t> values;
  std::unique_ptr<VulkanBufferObject> cpu_buffer;
  std::unique_ptr<VulkanBufferObject> gpu_buffer;
};

class LayoutBinding : public Node {
public:
  LayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage)
    : binding(binding), type(type), stage(stage) {}

  virtual ~LayoutBinding() {}

  virtual void traverse(RenderAction * action)
  {
    action->state.layout_binding.type = this->type;
    action->state.layout_binding.stage = this->stage;
    action->state.layout_binding.binding = this->binding;
  }
  
  uint32_t binding;
  VkDescriptorType type; 
  VkShaderStageFlags stage;
};

template <typename T>
class Buffer : public Node {
public:
  Buffer(VkBufferUsageFlagBits usage, VkMemoryPropertyFlags flags)
    : usage(usage), flags(flags), transfer(false)
  {}
  virtual ~Buffer() {}

  virtual void traverse(RenderAction * action)
  {
    size_t size = sizeof(T) * this->values.size();
    if (!this->buffer) {
      this->buffer = std::make_unique<VulkanBufferObject>(action->device, size, this->usage, this->flags);
      this->buffer->memory->memcpy(this->values.data(), size);
    } else if (this->transfer) {
      this->buffer->memory->memcpy(this->values.data(), size);
    }
    action->state.buffers.push_back({
      action->state.layout_binding.binding,             // binding 
      action->state.layout_binding.type,                // type
      action->state.layout_binding.stage,               // stage 
      this->buffer->buffer->buffer,                     // buffer 
      size,                                             // size 
      0                                                 // offset 
    });
  }

  bool transfer;
  std::vector<T> values;
  VkBufferUsageFlagBits usage;
  VkMemoryPropertyFlags flags;
  std::unique_ptr<VulkanBufferObject> buffer;
};

class VertexAttributeDescription : public Node {
public:
  VertexAttributeDescription(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset)
    : location(location), binding(binding), format(format), offset(offset), input_rate(VK_VERTEX_INPUT_RATE_VERTEX) 
  {}

  virtual ~VertexAttributeDescription() {}

  virtual void traverse(RenderAction * action)
  {
    action->state.attribute_description.location = this->location;
    action->state.attribute_description.binding = this->binding;
    action->state.attribute_description.format = this->format;
    action->state.attribute_description.offset = this->offset;
    action->state.attribute_description.input_rate = this->input_rate;
  }

  uint32_t location;
  uint32_t binding;
  VkFormat format;
  uint32_t offset;
  VkVertexInputRate input_rate;
};

template <typename T>
class VertexAttribute : public Node {
public:
  VertexAttribute() {}
  virtual ~VertexAttribute() {}

  virtual void traverse(RenderAction * action)
  {
    if (!this->gpu_buffer) {
      size_t size = sizeof(T) * this->values.size();
      this->cpu_buffer = std::make_unique<VulkanBufferObject>(action->device, 
                                                              size, 
                                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

      this->gpu_buffer = std::make_unique<VulkanBufferObject>(action->device, 
                                                              size, 
                                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

      this->cpu_buffer->memory->memcpy(this->values.data(), size);

      std::vector<VkBufferCopy> regions = {{
        0,    // srcOffset
        0,    // dstOffset 
        size, // size 
      }};

      vkCmdCopyBuffer(action->command->buffer(), 
                      this->cpu_buffer->buffer->buffer, 
                      this->gpu_buffer->buffer->buffer, 
                      static_cast<uint32_t>(regions.size()),
                      regions.data());
    }
    action->state.attribute_description.buffer = this->gpu_buffer->buffer->buffer;
    action->state.attribute_description.stride = sizeof(T);
    action->state.attributes.push_back(action->state.attribute_description);
  }

  std::vector<T> values;
  std::unique_ptr<VulkanBufferObject> cpu_buffer;
  std::unique_ptr<VulkanBufferObject> gpu_buffer;
};

class Shader : public Node {
public:
  Shader(const std::string & filename, VkShaderStageFlagBits stage)
    : filename(filename), stage(stage) {}

  virtual ~Shader() {}

  virtual void traverse(RenderAction * action)
  {
    if (!this->shader) {
      std::ifstream input(filename, std::ios::binary);
      std::vector<char> code((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
      this->shader = std::make_unique<VulkanShaderModule>(action->device, code);
    }

    VulkanShaderModuleDescription shader;
    shader.module = this->shader->module;
    shader.stage = this->stage;
    action->state.shaders.push_back(shader);
  }

  std::string filename;
  VkShaderStageFlagBits stage;
  std::unique_ptr<VulkanShaderModule> shader;
};

class Sampler : public Node {
public:
  Sampler() {}
  virtual ~Sampler() {}

  virtual void traverse(RenderAction * action)
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
  Texture(const std::string & filename)
    : Texture(gli::load(filename)) {}

  Texture(const gli::texture & texture)
    : texture(texture) {}

  virtual ~Texture() {}

  virtual void traverse(RenderAction * action)
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
  CullMode(VkCullModeFlags cullmode)
    : cullmode(cullmode) {}

  virtual ~CullMode() {}

  virtual void traverse(RenderAction * action)
  {
    action->state.rasterizationstate.cullMode = this->cullmode;
  }

  VkCullModeFlags cullmode;
};

class RasterizationState : public Node {
public:
  RasterizationState() 
    : state(State::defaultRasterizationState()) {}

  virtual ~RasterizationState() {}

  virtual void traverse(RenderAction * action)
  {
    action->state.rasterizationstate = this->state;
  }

  VkPipelineRasterizationStateCreateInfo state;
};

class Command : public Node {
public:
  std::unique_ptr<VulkanCommandBuffers> command;
};

class ComputeCommand : public Node {
public:
  ComputeCommand(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
    : group_count_x(group_count_x), group_count_y(group_count_y), group_count_z(group_count_z) {}

  virtual ~ComputeCommand() {}

  virtual void traverse(RenderAction * action)
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
  DrawCommand(VkPrimitiveTopology topology, uint32_t count = 0)
    : topology(topology),
      count(count),
      transform(std::make_shared<Buffer<mat4>>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)),
      transform_layout(std::make_shared<LayoutBinding>(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS))
  {
    this->transform->transfer = true;
  }

  virtual ~DrawCommand() {}

  virtual void traverse(RenderAction * action)
  {
    this->transform->values = {
      action->state.ViewMatrix * action->state.ModelMatrix, 
      action->state.ProjMatrix 
    };
    this->transform_layout->traverse(action);
    this->transform->traverse(action);

    action->state.drawdescription.count = this->count;
    action->state.drawdescription.topology = this->topology;
    action->graphic_states.push_back(action->state);
  }

  virtual void traverse(BoundingBoxAction * action)
  {
    box3 box(vec3(0), vec3(1));
    box.transform(action->state.ModelMatrix);
    action->extendBy(box);
  }

  uint32_t count;
  VkPrimitiveTopology topology;
  std::shared_ptr<Buffer<glm::mat4>> transform;
  std::shared_ptr<LayoutBinding> transform_layout;
  std::unique_ptr<class DescriptorSetObject> descriptor_set;
  std::unique_ptr<class GraphicsPipelineObject> pipeline;
  std::unique_ptr<VulkanCommandBuffers> command;
};

class Box : public DrawCommand {
public:
  Box(uint32_t binding, uint32_t location)
    : DrawCommand(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 36),
      indices(std::make_shared<IndexBuffer>()),
      attribute_description(std::make_shared<VertexAttributeDescription>(location, binding, VK_FORMAT_R32G32B32_SFLOAT, 0)),
      vertices(std::make_shared<VertexAttribute<glm::vec3>>())
  {
    //     5------7
    //    /|     /|
    //   / |    / |
    //  1------3  |
    //  |  4---|--6
    //  z x    | /
    //  |/     |/
    //  0--y---2

    this->vertices->values = {
      vec3(0, 0, 0), // 0
      vec3(0, 0, 1), // 1
      vec3(0, 1, 0), // 2
      vec3(0, 1, 1), // 3
      vec3(1, 0, 0), // 4
      vec3(1, 0, 1), // 5
      vec3(1, 1, 0), // 6
      vec3(1, 1, 1), // 7
    };

    this->indices->values = {
      0, 1, 3, 3, 2, 0, // -x
      4, 6, 7, 7, 5, 4, // +x
      0, 4, 5, 5, 1, 0, // -y
      6, 2, 3, 3, 7, 6, // +y
      0, 2, 6, 6, 4, 0, // -z
      1, 5, 7, 7, 3, 1, // +z
    };
  }

  virtual ~Box() {}

  virtual void traverse(RenderAction * action)
  {
    this->indices->traverse(action);
    this->attribute_description->traverse(action);
    this->vertices->traverse(action);
    DrawCommand::traverse(action);
  }

  virtual void traverse(BoundingBoxAction * action)
  {
    box3 box(vec3(0), vec3(1));
    box.transform(action->state.ModelMatrix);
    action->extendBy(box);
  }

  std::shared_ptr<IndexBuffer> indices;
  std::shared_ptr<VertexAttributeDescription> attribute_description;
  std::shared_ptr<VertexAttribute<glm::vec3>> vertices;
};


class Sphere : public DrawCommand {
public:
  Sphere(uint32_t binding, uint32_t location)
    : DrawCommand(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 36),
      indices(std::make_shared<IndexBuffer>()),
      attribute_description(std::make_shared<VertexAttributeDescription>(location, binding, VK_FORMAT_R32G32B32_SFLOAT, 0)),
      vertices(std::make_shared<VertexAttribute<vec3>>())
  {
    this->indices->values = {
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
    this->vertices->values = {
      vec3(-1, 0, t),
      vec3(1, 0, t),
      vec3(-1, 0,-t),
      vec3(1, 0,-t),
      vec3(0, t, 1),
      vec3(0, t,-1),
      vec3(0,-t, 1),
      vec3(0,-t,-1),
      vec3(t, 1, 0),
      vec3(-t, 1, 0),
      vec3(t,-1, 0),
      vec3(-t,-1, 0)
    };
  }

  virtual ~Sphere() {}

  virtual void traverse(RenderAction * action)
  {
    this->indices->traverse(action);
    this->attribute_description->traverse(action);
    this->vertices->traverse(action);
    DrawCommand::traverse(action);
  }

  virtual void traverse(BoundingBoxAction * action)
  {
    box3 box(vec3(0), vec3(1));
    box.transform(action->state.ModelMatrix);
    action->extendBy(box);
  }

  std::shared_ptr<IndexBuffer> indices;
  std::shared_ptr<VertexAttributeDescription> attribute_description;
  std::shared_ptr<VertexAttribute<glm::vec3>> vertices;
};