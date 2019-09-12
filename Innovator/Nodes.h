#pragma once

#include <Innovator/RenderManager.h>
#include <Innovator/Wrapper.h>
#include <Innovator/Node.h>
#include <Innovator/Defines.h>
#include <Innovator/Factory.h>

#include <vulkan/vulkan.h>
#include <shaderc/shaderc.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <utility>
#include <vector>
#include <memory>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

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

  explicit Separator(std::vector<std::shared_ptr<Node>> children) : 
    Group(std::move(children)) 
  {}

protected:
  void doAlloc(RenderManager * context) override
  {
    StateScope<RenderManager, State> scope(context);
    Group::doAlloc(context);
  }

  void doResize(RenderManager * context) override
  {
    StateScope<RenderManager, State> scope(context);
    Group::doResize(context);
  }

  void doStage(RenderManager * context) override
  {
    StateScope<RenderManager, State> scope(context);
    Group::doStage(context);
  }

  void doPipeline(RenderManager * creator) override
  {
    StateScope<RenderManager, State> scope(creator);
    Group::doPipeline(creator);
  }

  void doRecord(RenderManager * recorder) override
  {
    StateScope<RenderManager, State> scope(recorder);
    Group::doRecord(recorder);
  }

  void doRender(SceneRenderer * renderer) override
  {
    StateScope<SceneRenderer, RenderState> scope(renderer);
    Group::doRender(renderer);
  }

  void doPresent(RenderManager * context) override
  {
    StateScope<RenderManager, State> scope(context);
    Group::doPresent(context);
  }
};


class ViewMatrix : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(ViewMatrix)
  ViewMatrix() = delete;
  virtual ~ViewMatrix() = default;

  ViewMatrix(glm::dvec3 eye, glm::dvec3 target, glm::dvec3 up)
    : mat(glm::lookAt(eye, target, up))
  {}

  void zoom(double dy)
  {
    this->mat = glm::translate(this->mat, glm::dvec3(0.0, 0.0, dy));
  }

  void pan(const glm::dvec2& dx)
  {
    this->mat = glm::translate(this->mat, glm::dvec3(dx, 0.0));
  }

  void orbit(const glm::dvec2& dx)
  {
  }


private:
  void doRender(SceneRenderer* renderer) override
  {
    renderer->state.ViewMatrix = this->mat;
  }

  glm::dmat4 mat{ 1.0 };
};


class ProjMatrix : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(ProjMatrix)
  ProjMatrix() = delete;
  virtual ~ProjMatrix() = default;

  ProjMatrix(float farplane, float nearplane, float aspectratio, float fieldofview)
    : farplane(farplane),
      nearplane(nearplane),
      aspectratio(aspectratio),
      fieldofview(fieldofview)
  {
    this->mat = glm::perspective(this->fieldofview,
                                 this->aspectratio,
                                 this->nearplane,
                                 this->farplane);
  }

private:
  void doResize(RenderManager* context) override
  {
    this->aspectratio = static_cast<float>(context->extent.width) /
                        static_cast<float>(context->extent.height);

    this->mat = glm::perspective(this->fieldofview,
                                 this->aspectratio,
                                 this->nearplane,
                                 this->farplane);
  }

  void doRender(SceneRenderer* renderer) override
  {
    renderer->state.ProjMatrix = this->mat;
  }

  glm::dmat4 mat;

  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;
};


class Transform : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Transform)
  Transform() = default;
  virtual ~Transform() = default;

  explicit Transform(const glm::dvec3 & t,
                     const glm::dvec3 & s)
  {
    this->matrix = glm::scale(this->matrix, s);
    this->matrix = glm::translate(this->matrix, t);
  }

private:
  void doRender(SceneRenderer * renderer) override
  {
    renderer->state.ModelMatrix *= this->matrix;
  }

  glm::dmat4 matrix{ 1.0 };
};

class BufferData : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(BufferData)
  BufferData() = default;
  virtual ~BufferData() = default;

  virtual void copy(char * dst) const = 0;
  virtual size_t size() const = 0;
  virtual size_t stride() const = 0;
  size_t count() const 
  {
    return this->size() / this->stride();
  }

private:
  void doAlloc(RenderManager * context) override
  {
    context->state.bufferdata = this;
  }

  void doStage(RenderManager * context) override
  {
    context->state.bufferdata = this;
  }

  void doPipeline(RenderManager * context) override
  {
    context->state.bufferdata = this;
  }

  void doRecord(RenderManager * context) override
  {
    context->state.bufferdata = this;
  }
};

template <typename T>
class InlineBufferData : public BufferData {
public:
  NO_COPY_OR_ASSIGNMENT(InlineBufferData)
  InlineBufferData() = default;
  virtual ~InlineBufferData() = default;

  explicit InlineBufferData(std::vector<T> values) :
    values(std::move(values))
  {}

  void copy(char * dst) const override
  {
    std::copy(this->values.begin(),
              this->values.end(),
              reinterpret_cast<T*>(dst));
  }

  size_t size() const override
  {
    return this->values.size() * sizeof(T);
  }

  size_t stride() const override
  {
    return sizeof(T);
  }

  std::vector<T> values;
};

class STLBufferData : public BufferData {
public:
  NO_COPY_OR_ASSIGNMENT(STLBufferData)
  STLBufferData() = delete;
  virtual ~STLBufferData() = default;

  explicit STLBufferData(std::string filename) :
    filename(std::move(filename))
  {
    std::uintmax_t file_size = fs::file_size(fs::current_path() / this->filename);
    std::cout << "std::file_size reported size of: " << file_size << std::endl;

    size_t num_triangles = (file_size - 84) / 50;
    std::cout << "num triangles should be " << num_triangles << std::endl;

    this->values_size = num_triangles * 36;
  }

  void copy(char * dst) const override
  {
    std::ifstream input(this->filename, std::ios::binary);
    // header is first 80 bytes
    char header[80];
    input.read(header, 80);
    std::cout << "header: " << std::string(header) << std::endl;

    // num triangles is next 4 bytes after header
    uint32_t num_triangles;
    input.read(reinterpret_cast<char*>(&num_triangles), 4);
    std::cout << "num triangles: " << num_triangles << std::endl;

    char normal[12];
    char attrib[2];
    for (size_t i = 0; i < num_triangles; i++) {
      input.read(normal, 12); // skip normal
      input.read(dst + i * 36, 36);
      input.read(attrib, 2);  // skip attribute
    }
  }

  std::string filename;
  size_t values_size;

  size_t size() const override
  {
    return this->values_size;
  }

  size_t stride() const override
  {
    return sizeof(float);
  }
};

class CpuMemoryBuffer : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(CpuMemoryBuffer)
  CpuMemoryBuffer() = delete;
  virtual ~CpuMemoryBuffer() = default;

  explicit CpuMemoryBuffer(VkBufferUsageFlags usage_flags, 
                           VkBufferCreateFlags create_flags = 0) :
    usage_flags(usage_flags), 
    create_flags(create_flags)
  {}

private:
  void doAlloc(RenderManager * context) override
  {
    this->buffer = std::make_shared<BufferObject>(
      std::make_shared<VulkanBuffer>(context->device,
                                     this->create_flags,
                                     context->state.bufferdata->size(),
                                     this->usage_flags,
                                     VK_SHARING_MODE_EXCLUSIVE),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    context->bufferobjects.push_back(this->buffer);
  }

  void doStage(RenderManager * context) override
  {
    context->state.buffer = this->buffer->buffer->buffer;
    MemoryMap memmap(this->buffer->memory.get(), context->state.bufferdata->size(), this->buffer->offset);
    context->state.bufferdata->copy(memmap.mem);
  }

  void doPipeline(RenderManager * creator) override
  {
    creator->state.buffer = this->buffer->buffer->buffer;
  }

  void doRecord(RenderManager * recorder) override
  {
    recorder->state.buffer = this->buffer->buffer->buffer;
  }

  VkBufferUsageFlags usage_flags;
  VkBufferCreateFlags create_flags;
  std::shared_ptr<BufferObject> buffer{ nullptr };
};

class GpuMemoryBuffer : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(GpuMemoryBuffer)
  GpuMemoryBuffer() = delete;
  virtual ~GpuMemoryBuffer() = default;

  explicit GpuMemoryBuffer(VkBufferUsageFlags usage_flags, VkBufferCreateFlags create_flags = 0) : 
    usage_flags(usage_flags), 
    create_flags(create_flags)
  {}

private:
  void doAlloc(RenderManager * context) override
  {
    this->buffer = std::make_shared<BufferObject>(
      std::make_shared<VulkanBuffer>(context->device,
                                     this->create_flags,
                                     context->state.bufferdata->size(),
                                     this->usage_flags,
                                     VK_SHARING_MODE_EXCLUSIVE),
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    context->bufferobjects.push_back(this->buffer);
  }

  void doStage(RenderManager * context) override
  {
    std::vector<VkBufferCopy> regions = { {
        0,                                                   // srcOffset
        0,                                                   // dstOffset
        context->state.bufferdata->size(),                   // size
    } };

    vkCmdCopyBuffer(context->command->buffer(),
                    context->state.buffer,
                    this->buffer->buffer->buffer,
                    static_cast<uint32_t>(regions.size()),
                    regions.data());
  }

  void doPipeline(RenderManager * creator) override
  {
    creator->state.buffer = this->buffer->buffer->buffer;
  }

  void doRecord(RenderManager * recorder) override
  {
    recorder->state.buffer = this->buffer->buffer->buffer;
  }

  VkBufferUsageFlags usage_flags;
  VkBufferCreateFlags create_flags;
  std::shared_ptr<BufferObject> buffer{ nullptr };
};

class TransformBuffer : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(TransformBuffer)
  virtual ~TransformBuffer() = default;

  TransformBuffer()
    : size(sizeof(glm::mat4) * 2)
  {}

private:
  void doAlloc(RenderManager * context) override
  {
    this->buffer = std::make_shared<BufferObject>(
      std::make_shared<VulkanBuffer>(context->device,
                                     0,                                     
                                     this->size,
                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_SHARING_MODE_EXCLUSIVE),
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    context->bufferobjects.push_back(this->buffer);
  }

  void doPipeline(RenderManager * creator) override
  {
    creator->state.buffer = this->buffer->buffer->buffer;
  }

  void doRender(SceneRenderer * renderer) override
  {
    std::array<glm::mat4, 2> data = {
      glm::mat4(renderer->state.ViewMatrix * renderer->state.ModelMatrix),
      glm::mat4(renderer->state.ProjMatrix)
    };

    MemoryMap map(this->buffer->memory.get(), this->size, this->buffer->offset);
    std::copy(data.begin(), data.end(), reinterpret_cast<glm::mat4*>(map.mem));
  }

  size_t size;
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
  void doRecord(RenderManager * recorder) override
  {
    recorder->state.index_buffer_description = {
      this->type,
      recorder->state.buffer
    };
  }

  VkIndexType type;
};

class VertexInputAttributeDescription : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(VertexInputAttributeDescription)
  VertexInputAttributeDescription() = delete;
  virtual ~VertexInputAttributeDescription() = default;

  explicit VertexInputAttributeDescription(uint32_t location,
                                           uint32_t binding,
                                           VkFormat format,
                                           uint32_t offset) :
    vertex_input_attribute_description({ 
      location, binding, format, offset 
    })
  {}

private:
  void doPipeline(RenderManager * creator) override
  {
    creator->state.vertex_attributes.push_back(this->vertex_input_attribute_description);
  }

  void doRecord(RenderManager * recorder) override
  {
    recorder->state.vertex_attribute_buffers.push_back(recorder->state.buffer);
    recorder->state.vertex_attribute_buffer_offsets.push_back(0);
  }

  VkVertexInputAttributeDescription vertex_input_attribute_description;
};

class VertexInputBindingDescription : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(VertexInputBindingDescription)
  VertexInputBindingDescription() = delete;
  virtual ~VertexInputBindingDescription() = default;

  explicit VertexInputBindingDescription(uint32_t binding,
                                         uint32_t stride,
                                         VkVertexInputRate inputRate) :
    binding(binding),
    stride(stride),
    inputRate(inputRate)
  {}

private:
  void doPipeline(RenderManager * creator) override
  {
    creator->state.vertex_input_bindings.push_back({
      this->binding,
      this->stride,
      this->inputRate,
    });
  }
  
  uint32_t binding;
  uint32_t stride;
  VkVertexInputRate inputRate;
};

class DescriptorSetLayoutBinding : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(DescriptorSetLayoutBinding)
  DescriptorSetLayoutBinding() = delete;
  virtual ~DescriptorSetLayoutBinding() = default;

  DescriptorSetLayoutBinding(uint32_t binding, 
                             VkDescriptorType descriptorType, 
                             VkShaderStageFlags stageFlags) :
    binding(binding),
    descriptorType(descriptorType),
    stageFlags(stageFlags)
  {}

private:
  void doPipeline(RenderManager * creator) override
  {
    creator->state.descriptor_pool_sizes.push_back({
      this->descriptorType,                // type 
      1,                                   // descriptorCount
    });

    creator->state.descriptor_set_layout_bindings.push_back({
      this->binding,
      this->descriptorType,
      1,
      this->stageFlags,
      nullptr,
    });

    this->descriptor_image_info = {
      creator->state.sampler,
      creator->state.imageView,
      creator->state.imageLayout
    };

    this->descriptor_buffer_info = {
      creator->state.buffer,                          // buffer
      0,                                              // offset
      VK_WHOLE_SIZE                                   // range
    };
   
    creator->state.write_descriptor_sets.push_back({
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,          // sType
      nullptr,                                         // pNext
      nullptr,                                         // dstSet
      this->binding,                                   // dstBinding
      0,                                               // dstArrayElement
      1,                                               // descriptorCount
      this->descriptorType,                            // descriptorType
      &this->descriptor_image_info,                    // pImageInfo
      &this->descriptor_buffer_info,                   // pBufferInfo
      nullptr,                                         // pTexelBufferView
    });
  }

  uint32_t binding;
  VkDescriptorType descriptorType;
  VkShaderStageFlags stageFlags;
  VkDescriptorImageInfo descriptor_image_info{};
  VkDescriptorBufferInfo descriptor_buffer_info{};
};

class Shader : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Shader)
  Shader() = delete;
  virtual ~Shader() = default;

  explicit Shader(std::string filename, const VkShaderStageFlagBits stage):
    stage(stage)
  {
    std::ifstream input(filename, std::ios::in);
    std::string glsl(std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{});

    shaderc_shader_kind kind = [stage]() 
    {
      switch (stage) {
      case VK_SHADER_STAGE_VERTEX_BIT:
        return shaderc_vertex_shader;
      case VK_SHADER_STAGE_COMPUTE_BIT:
        return shaderc_compute_shader;
      case VK_SHADER_STAGE_GEOMETRY_BIT:
        return shaderc_geometry_shader;
      case VK_SHADER_STAGE_FRAGMENT_BIT:
        return shaderc_fragment_shader;
      case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return shaderc_tess_control_shader;
      case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return shaderc_tess_evaluation_shader;
      default:
        throw std::runtime_error("Unknown shader stage");
      }
    }();

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(glsl, kind, filename.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
      throw std::runtime_error(module.GetErrorMessage());
    }
    this->spv = { module.cbegin(), module.cend() };
  }

private:
  void doAlloc(RenderManager * context) override
  {
    this->shader = std::make_unique<VulkanShaderModule>(context->device, this->spv);
  }

  void doPipeline(RenderManager * creator) override
  {
    creator->state.shader_stage_infos.push_back({
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, // sType 
      nullptr,                                             // pNext
      0,                                                   // flags (reserved for future use)
      this->stage,                                         // stage
      this->shader->module,                                // module 
      "main",                                              // pName 
      nullptr,                                             // pSpecializationInfo
    });
  }

protected:
  std::vector<uint32_t> spv;
  VkShaderStageFlagBits stage;
  std::unique_ptr<VulkanShaderModule> shader;
};

class Sampler : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Sampler)
  virtual ~Sampler() = default;
  
  Sampler(VkFilter mag_filter,
          VkFilter min_filter,
          VkSamplerMipmapMode mipmap_mode,
          VkSamplerAddressMode address_mode_u,
          VkSamplerAddressMode address_mode_v,
          VkSamplerAddressMode address_mode_w) :
    mag_filter(mag_filter),
    min_filter(min_filter),
    mipmap_mode(mipmap_mode),
    address_mode_u(address_mode_u),
    address_mode_v(address_mode_v),
    address_mode_w(address_mode_w),
    mip_lod_bias(0.0f)
  {}

private:
  void doAlloc(RenderManager * context) override
  {
    this->sampler = std::make_unique<VulkanSampler>(context->device,
                                                    this->mag_filter,
                                                    this->min_filter,
                                                    this->mipmap_mode,
                                                    this->address_mode_u,
                                                    this->address_mode_v,
                                                    this->address_mode_w,
                                                    this->mip_lod_bias,
                                                    VK_FALSE,
                                                    1.f,
                                                    VK_FALSE,
                                                    VK_COMPARE_OP_NEVER,
                                                    0.f,
                                                    0.f,
                                                    VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
                                                    VK_FALSE);
  }

  void doPipeline(RenderManager * creator) override
  {
    creator->state.sampler = this->sampler->sampler;
  }

  std::unique_ptr<VulkanSampler> sampler;
  VkFilter mag_filter;
  VkFilter min_filter;
  VkSamplerMipmapMode mipmap_mode;
  VkSamplerAddressMode address_mode_u;
  VkSamplerAddressMode address_mode_v;
  VkSamplerAddressMode address_mode_w;
  float mip_lod_bias;
};

class TextureImage : public BufferData {
public:
  NO_COPY_OR_ASSIGNMENT(TextureImage)
  TextureImage() = delete;
  virtual ~TextureImage() = default;

  explicit TextureImage(const std::string& filename) :
    texture(VulkanImageFactory::Create(filename))
  {}

  void copy(char* dst) const override
  {
    std::copy(this->texture->data(), this->texture->data() + this->texture->size(), dst);
  }

  size_t size() const override
  {
    return this->texture->size();
  }

  size_t stride() const override
  {
    return 0;
  }

private:
  void doAlloc(RenderManager* context) override
  {
    context->state.bufferdata = this;
    context->state.texture = this->texture.get();
  }

  void doStage(RenderManager* context) override
  {
    context->state.bufferdata = this;
    context->state.texture = this->texture.get();
  }

  void doPipeline(RenderManager* context) override
  {
    context->state.bufferdata = this;
    context->state.texture = this->texture.get();
  }

  void doRecord(RenderManager* context) override
  {
    context->state.bufferdata = this;
    context->state.texture = this->texture.get();
  }

  std::shared_ptr<VulkanTextureImage> texture;
};


class Image : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Image)
  virtual ~Image() = default;
  Image(VkSampleCountFlagBits sample_count,
        VkImageTiling tiling,
        VkImageUsageFlags usage_flags,
        VkSharingMode sharing_mode,
        VkImageCreateFlags create_flags,
        VkImageLayout layout) :
    sample_count(sample_count),
    tiling(tiling),
    usage_flags(usage_flags),
    sharing_mode(sharing_mode),
    create_flags(create_flags),
    layout(layout)
  {}

private:
  void doAlloc(RenderManager* context) override
  {
    this->image = std::make_shared<VulkanImage>(context->device,
                                                context->state.texture->image_type(),
                                                context->state.texture->format(),
                                                context->state.texture->extent(0),
                                                context->state.texture->levels(),
                                                context->state.texture->layers(),
                                                this->sample_count,
                                                this->tiling,
                                                this->usage_flags,
                                                this->sharing_mode,
                                                this->create_flags);

    this->image_object = std::make_shared<ImageObject>(this->image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    context->imageobjects.push_back(this->image_object);
  }

  void doStage(RenderManager* context) override
  {
    context->state.image = this->image->image;
    VulkanTextureImage* texture = context->state.texture;

    {
      VkImageMemoryBarrier memory_barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                // sType
        nullptr,                                               // pNext
        0,                                                     // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_UNDEFINED,                             // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                  // newLayout
        0,                                                     // srcQueueFamilyIndex
        0,                                                     // dstQueueFamilyIndex
        this->image->image,                                    // image
        texture->subresource_range(),                          // subresourceRange
      };

      vkCmdPipelineBarrier(context->command->buffer(),
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 0, nullptr, 1,
                           &memory_barrier);
    }

    std::vector<VkBufferImageCopy> regions;

    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < texture->levels(); mip_level++) {

      const VkImageSubresourceLayers subresource_layers{
        texture->subresource_range().aspectMask,               // aspectMask
        mip_level,                                             // mipLevel
        texture->subresource_range().baseArrayLayer,           // baseArrayLayer
        texture->subresource_range().layerCount,               // layerCount
      };

      regions.push_back({
        buffer_offset,                             // bufferOffset 
        0,                                         // bufferRowLength
        0,                                         // bufferImageHeight
        subresource_layers,                        // imageSubresource
        { 0, 0, 0 },                               // imageOffset
        texture->extent(mip_level),                // imageExtent
        });

      buffer_offset += texture->size(mip_level);
    }

    vkCmdCopyBufferToImage(context->command->buffer(),
                           context->state.buffer,
                           this->image->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(regions.size()),
                           regions.data());
    {
      VkImageMemoryBarrier memory_barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                // sType
        nullptr,                                               // pNext
        0,                                                     // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                  // oldLayout
        this->layout,                                          // newLayout
        0,                                                     // srcQueueFamilyIndex
        0,                                                     // dstQueueFamilyIndex
        this->image->image,                                    // image
        texture->subresource_range(),                          // subresourceRange
      };

      vkCmdPipelineBarrier(context->command->buffer(),
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 0, nullptr, 1,
                           &memory_barrier);
    }
  }

  void doPipeline(RenderManager * context) override
  {
    context->state.imageLayout = this->layout;
  }

  std::shared_ptr<VulkanImage> image;
  std::shared_ptr<ImageObject> image_object;

  VkSampleCountFlagBits sample_count;
  VkImageTiling tiling;
  VkImageUsageFlags usage_flags;
  VkSharingMode sharing_mode;
  VkImageCreateFlags create_flags;
  VkImageLayout layout;
};


class ImageView : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(ImageView)
  virtual ~ImageView() = default;
  ImageView(VkComponentSwizzle r,
            VkComponentSwizzle g,
            VkComponentSwizzle b,
            VkComponentSwizzle a) :
    component_mapping({ r, g, b, a })
  {}

private:
  void doStage(RenderManager* context) override
  {
    this->view = std::make_unique<VulkanImageView>(context->device,
                                                   context->state.image,
                                                   context->state.texture->format(),
                                                   context->state.texture->image_view_type(),
                                                   this->component_mapping,
                                                   context->state.texture->subresource_range());
  }

  void doPipeline(RenderManager* context) override
  {
    context->state.imageView = this->view->view;
  }

  std::unique_ptr<VulkanImageView> view;
  VkComponentMapping component_mapping;
};

class CullMode : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(CullMode)
    CullMode() = delete;
  virtual ~CullMode() = default;

  explicit CullMode(VkCullModeFlags cullmode) :
    cullmode(cullmode)
  {}
  
private:
  void doPipeline(RenderManager * creator) override
  {
    creator->state.rasterization_state.cullMode = this->cullmode;
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
  void doPipeline(RenderManager * creator) override
  {
    this->descriptor_set_layout = std::make_unique<VulkanDescriptorSetLayout>(
      creator->device,
      creator->state.descriptor_set_layout_bindings);

    this->descriptor_set = std::make_unique<VulkanDescriptorSets>(
      creator->device,
      this->descriptor_pool,
      std::vector<VkDescriptorSetLayout>{ this->descriptor_set_layout->layout });

    this->pipeline_layout = std::make_unique<VulkanPipelineLayout>(
      creator->device,
      std::vector<VkDescriptorSetLayout>{ this->descriptor_set_layout->layout },
      creator->state.push_constant_ranges);

    for (auto & write_descriptor_set : creator->state.write_descriptor_sets) {
      write_descriptor_set.dstSet = this->descriptor_set->descriptor_sets[0];
    }
    this->descriptor_set->update(creator->state.write_descriptor_sets);

    this->pipeline = std::make_unique<VulkanComputePipeline>(
      creator->device,
      creator->pipelinecache->cache,
      creator->state.shader_stage_infos[0],
      this->pipeline_layout->layout);
  }

  void doRecord(RenderManager * recorder) override
  {
    vkCmdBindDescriptorSets(this->command->buffer(),
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            this->pipeline_layout->layout,
                            0,
                            static_cast<uint32_t>(this->descriptor_set->descriptor_sets.size()),
                            this->descriptor_set->descriptor_sets.data(),
                            0,
                            nullptr);

    vkCmdBindPipeline(this->command->buffer(),
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      this->pipeline->pipeline);

    vkCmdDispatch(this->command->buffer(),
                  this->group_count_x,
                  this->group_count_y,
                  this->group_count_z);
  }

  uint32_t group_count_x;
  uint32_t group_count_y;
  uint32_t group_count_z;

  std::unique_ptr<VulkanComputePipeline> pipeline;
  std::unique_ptr<VulkanCommandBuffers> command;

  std::shared_ptr<VulkanDescriptorSetLayout> descriptor_set_layout;
  std::shared_ptr<VulkanDescriptorSets> descriptor_set;
  std::shared_ptr<VulkanPipelineLayout> pipeline_layout;
  std::shared_ptr<VulkanDescriptorPool> descriptor_pool;
};

class DrawCommandBase : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(DrawCommandBase)
  DrawCommandBase() = delete;
  virtual ~DrawCommandBase() = default;

  explicit DrawCommandBase(VkPrimitiveTopology topology) : 
    topology(topology)
  {}

private:
  virtual void execute(VkCommandBuffer command, RenderManager * recorder) = 0;

  void doAlloc(RenderManager * context) override
  {
    this->command = std::make_unique<VulkanCommandBuffers>(
      context->device,
      1,
      VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  }

  void doPipeline(RenderManager * creator) override
  {
    auto descriptor_pool = std::make_shared<VulkanDescriptorPool>(
      creator->device,
      creator->state.descriptor_pool_sizes);

    this->descriptor_set_layout = std::make_unique<VulkanDescriptorSetLayout>(
      creator->device,
      creator->state.descriptor_set_layout_bindings);

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts{ 
      this->descriptor_set_layout->layout 
    };

    this->pipeline_layout = std::make_unique<VulkanPipelineLayout>(
      creator->device,
      descriptor_set_layouts,
      creator->state.push_constant_ranges);

    this->descriptor_sets = std::make_unique<VulkanDescriptorSets>(
      creator->device,
      descriptor_pool,
      descriptor_set_layouts);

    for (auto & write_descriptor_set : creator->state.write_descriptor_sets) {
      write_descriptor_set.dstSet = this->descriptor_sets->descriptor_sets[0];
    }
    this->descriptor_sets->update(creator->state.write_descriptor_sets);

    this->pipeline = std::make_unique<VulkanGraphicsPipeline>(
      creator->device,
      creator->state.renderpass->renderpass,
      creator->pipelinecache->cache,
      this->pipeline_layout->layout,
      this->topology,
      creator->state.rasterization_state,
      this->dynamic_states,
      creator->state.shader_stage_infos,
      creator->state.vertex_input_bindings,
      creator->state.vertex_attributes);
  }

  void doRecord(RenderManager * recorder) override
  {
    VulkanCommandBufferScope command_scope(this->command->buffer(),
                                           recorder->state.renderpass->renderpass,
                                           0,
                                           VK_NULL_HANDLE,
                                           VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

    //glm::mat4 mv(1.0);
    //vkCmdPushConstants(this->command->buffer(),
    //                   this->pipeline_layout->layout,
    //                   VK_SHADER_STAGE_VERTEX_BIT,
    //                   0,
    //                   static_cast<uint32_t>(sizeof(glm::mat4)),
    //                   glm::value_ptr(mv));

    vkCmdBindDescriptorSets(this->command->buffer(), 
                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            this->pipeline_layout->layout, 
                            0, 
                            static_cast<uint32_t>(this->descriptor_sets->descriptor_sets.size()), 
                            this->descriptor_sets->descriptor_sets.data(), 
                            0, 
                            nullptr);

    vkCmdBindPipeline(this->command->buffer(), 
                      VK_PIPELINE_BIND_POINT_GRAPHICS, 
                      this->pipeline->pipeline);

    std::vector<VkRect2D> scissors{ {
      { 0, 0 },
      recorder->extent
    } };

    std::vector<VkViewport> viewports{ {
      0.0f,                                         // x
      0.0f,                                         // y
      static_cast<float>(recorder->extent.width),   // width
      static_cast<float>(recorder->extent.height),  // height
      0.0f,                                         // minDepth
      1.0f                                          // maxDepth
    } };

    vkCmdSetScissor(this->command->buffer(), 
                    0, 
                    static_cast<uint32_t>(scissors.size()), 
                    scissors.data());

    vkCmdSetViewport(this->command->buffer(), 
                     0, 
                     static_cast<uint32_t>(viewports.size()),
                     viewports.data());

    vkCmdBindVertexBuffers(this->command->buffer(), 
                           0, 
                           static_cast<uint32_t>(recorder->state.vertex_attribute_buffers.size()),
                           recorder->state.vertex_attribute_buffers.data(),
                           recorder->state.vertex_attribute_buffer_offsets.data());
    
    this->execute(this->command->buffer(), recorder);
  }

  void doRender(SceneRenderer * renderer) override
  {
    vkCmdExecuteCommands(renderer->command->buffer(), 
                         static_cast<uint32_t>(this->command->buffers.size()), 
                         this->command->buffers.data());
  }

  VkPrimitiveTopology topology;
  std::unique_ptr<VulkanCommandBuffers> command;
  std::unique_ptr<VulkanGraphicsPipeline> pipeline;
  std::vector<VkDynamicState> dynamic_states{
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };
  std::shared_ptr<VulkanDescriptorSetLayout> descriptor_set_layout;
  std::shared_ptr<VulkanDescriptorSets> descriptor_sets;
  std::shared_ptr<VulkanPipelineLayout> pipeline_layout;
};

class DrawCommand : public DrawCommandBase {
public:
  NO_COPY_OR_ASSIGNMENT(DrawCommand)
  virtual ~DrawCommand() = default;

  explicit DrawCommand(uint32_t vertexcount,
                       uint32_t instancecount,
                       uint32_t firstvertex,
                       uint32_t firstinstance,
                       VkPrimitiveTopology topology) :
    DrawCommandBase(topology),
    vertexcount(vertexcount),
    instancecount(instancecount),
    firstvertex(firstvertex),
    firstinstance(firstinstance)
  {}

private:
  void execute(VkCommandBuffer command, RenderManager *) override
  {
    vkCmdDraw(command, this->vertexcount, this->instancecount, this->firstvertex, this->firstinstance);
  }

  uint32_t vertexcount;
  uint32_t instancecount;
  uint32_t firstvertex;
  uint32_t firstinstance;
};

class IndexedDrawCommand : public DrawCommandBase {
public:
  NO_COPY_OR_ASSIGNMENT(IndexedDrawCommand)
  virtual ~IndexedDrawCommand() = default;

  explicit IndexedDrawCommand(uint32_t indexcount,
                              uint32_t instancecount,
                              uint32_t firstindex,
                              int32_t vertexoffset,
                              uint32_t firstinstance,
                              VkPrimitiveTopology topology) :
    DrawCommandBase(topology),
    indexcount(indexcount),
    instancecount(instancecount),
    firstindex(firstindex),
    vertexoffset(vertexoffset),
    firstinstance(firstinstance),
    offset(0)
  {}

private:
  void execute(VkCommandBuffer command, RenderManager * recorder) override
  {
    vkCmdBindIndexBuffer(command, 
                         recorder->state.index_buffer_description.buffer, 
                         this->offset, 
                         recorder->state.index_buffer_description.type);

    vkCmdDrawIndexed(command, 
                     this->indexcount, 
                     this->instancecount, 
                     this->firstindex, 
                     this->vertexoffset, 
                     this->firstinstance);
  }

  uint32_t indexcount;
  uint32_t instancecount;
  uint32_t firstindex;
  int32_t vertexoffset;
  uint32_t firstinstance;
  VkDeviceSize offset;
};

class FramebufferAttachment {
public:
  NO_COPY_OR_ASSIGNMENT(FramebufferAttachment)
  virtual ~FramebufferAttachment() = default;

  FramebufferAttachment(VkFormat format,
                        VkImageUsageFlags usage,
                        VkImageAspectFlags aspectMask) :
    format(format),
    usage(usage)
  {
    this->subresource_range = {
      aspectMask, 0, 1, 0, 1
    };
  }

  VkImageView alloc(RenderManager * context)
  {
    VkExtent3D extent = { context->extent.width, context->extent.height, 1 };
    this->image = std::make_shared<VulkanImage>(context->device,
                                                VK_IMAGE_TYPE_2D,
                                                this->format,
                                                extent,
                                                this->subresource_range.levelCount,
                                                this->subresource_range.layerCount,
                                                VK_SAMPLE_COUNT_1_BIT,
                                                VK_IMAGE_TILING_OPTIMAL,
                                                this->usage,
                                                VK_SHARING_MODE_EXCLUSIVE);

    this->imageobject = std::make_shared<ImageObject>(this->image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    const auto memory = std::make_shared<VulkanMemory>(context->device,
                                                       this->imageobject->memory_requirements.size,
                                                       this->imageobject->memory_type_index);
    const VkDeviceSize offset = 0;
    this->imageobject->bind(memory, offset);
    this->imageview = std::make_shared<VulkanImageView>(context->device,
                                                        this->image->image,
                                                        this->format,
                                                        VK_IMAGE_VIEW_TYPE_2D,
                                                        this->component_mapping,
                                                        this->subresource_range);
    return this->imageview->view;
  }

public:
  VkFormat format;
  VkImageUsageFlags usage;
  VkImageSubresourceRange subresource_range;

  VkComponentMapping component_mapping{
    VK_COMPONENT_SWIZZLE_R,
    VK_COMPONENT_SWIZZLE_G,
    VK_COMPONENT_SWIZZLE_B,
    VK_COMPONENT_SWIZZLE_A
  };

public:
  std::shared_ptr<VulkanImage> image;
  std::shared_ptr<ImageObject> imageobject;
  std::shared_ptr<VulkanImageView> imageview;
};

class FramebufferObject : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(FramebufferObject)
  virtual ~FramebufferObject() = default;

  FramebufferObject(std::vector<std::shared_ptr<FramebufferAttachment>> attachments)
    : attachments(attachments)
  {}

private:
  void doAlloc(RenderManager* context) override
  {
    std::vector<VkImageView> imageviews;
    for (auto attachment : this->attachments) {
      imageviews.push_back(attachment->alloc(context));
    }

    this->framebuffer = std::make_unique<VulkanFramebuffer>(context->device,
                                                            context->state.renderpass,
                                                            imageviews,
                                                            context->extent,
                                                            1);
  }

  void doResize(RenderManager * context) override
  {
    this->doAlloc(context);
  }

public:
  std::unique_ptr<VulkanFramebuffer> framebuffer;
  std::vector<std::shared_ptr<FramebufferAttachment>> attachments;
};

class SubpassObject {
public:
  NO_COPY_OR_ASSIGNMENT(SubpassObject)

    SubpassObject(VkSubpassDescriptionFlags flags,
                  VkPipelineBindPoint bind_point,
                  std::vector<VkAttachmentReference> input_attachments,
                  std::vector<VkAttachmentReference> color_attachments,
                  std::vector<VkAttachmentReference> resolve_attachments,
                  VkAttachmentReference depth_stencil_attachment,
                  std::vector<uint32_t> preserve_attachments) :
    input_attachments(input_attachments),
    color_attachments(color_attachments),
    resolve_attachments(resolve_attachments),
    depth_stencil_attachment(depth_stencil_attachment),
    preserve_attachments(preserve_attachments)
  {
    this->description = {
      flags,                                                    // flags
      bind_point,                                               // pipelineBindPoint
      static_cast<uint32_t>(this->input_attachments.size()),    // inputAttachmentCount
      this->input_attachments.data(),                           // pInputAttachments
      static_cast<uint32_t>(this->color_attachments.size()),    // colorAttachmentCount
      this->color_attachments.data(),                           // pColorAttachments
      this->resolve_attachments.data(),                         // pResolveAttachments
      &this->depth_stencil_attachment,                          // pDepthStencilAttachment
      static_cast<uint32_t>(this->preserve_attachments.size()), // preserveAttachmentCount
      this->preserve_attachments.data()                         // pPreserveAttachments
    };
  }

  VkSubpassDescription description;

private:
  std::vector<VkAttachmentReference> input_attachments;
  std::vector<VkAttachmentReference> color_attachments;
  std::vector<VkAttachmentReference> resolve_attachments;
  VkAttachmentReference depth_stencil_attachment;
  std::vector<uint32_t> preserve_attachments;
};

class RenderpassObject : public Group {
public:
  NO_COPY_OR_ASSIGNMENT(RenderpassObject)
  virtual ~RenderpassObject() = default;

  RenderpassObject(std::vector<VkAttachmentDescription> attachments,
                   std::vector<std::shared_ptr<SubpassObject>> subpasses) :
    attachments(std::move(attachments)),
    subpasses(std::move(subpasses))
  {
    for (auto & subpass : this->subpasses) {
      this->subpass_descriptions.push_back(subpass->description);
    }
  }

private:
  void doAlloc(RenderManager* context) override
  {
    this->render_command = std::make_unique<VulkanCommandBuffers>(context->device);
    this->render_queue = context->device->getQueue(VK_QUEUE_GRAPHICS_BIT);
    this->render_fence = std::make_unique<VulkanFence>(context->device);
    this->rendering_finished = std::make_unique<VulkanSemaphore>(context->device);

    this->renderpass = std::make_shared<VulkanRenderpass>(context->device,
                                                          this->attachments,
                                                          this->subpass_descriptions);

    context->state.renderpass = this->renderpass;
    Group::doAlloc(context);
  }

  void doResize(RenderManager* context) override
  {
    context->state.renderpass = this->renderpass;
    Group::doResize(context);
  }

  void doPipeline(RenderManager * context) override
  {
    context->state.renderpass = this->renderpass;
    Group::doPipeline(context);
  }

  void doRecord(RenderManager * recorder) override
  {
    recorder->state.renderpass = this->renderpass;
    Group::doRecord(recorder);
  }

  void doRender(SceneRenderer * renderer) override
  {
    if (this->children.empty()) {
      throw std::runtime_error("RenderpassObject::doRender(): Nothing to render!");
    }

    std::shared_ptr<FramebufferObject> framebuffer = 
      std::dynamic_pointer_cast<FramebufferObject>(this->children[0]);

    if (!framebuffer) {
      throw std::runtime_error("RenderpassObject::doRender(): Renderpass does not contain a framebuffer!");
    }

    const VkRect2D renderarea{
      { 0, 0 },                 // offset
      renderer->extent          // extent
    };

    const std::vector<VkClearValue> clearvalues{
      { { { 0.0f, 0.0f, 0.0f, 0.0f } } },
      { { { 1.0f, 0 } } }
    };

    {
      renderer->command = this->render_command.get();

      VulkanCommandBufferScope commandbuffer(this->render_command->buffer());

      VulkanRenderPassScope renderpass_scope(this->renderpass->renderpass,
                                             framebuffer->framebuffer->framebuffer,
                                             renderarea,
                                             clearvalues,
                                             this->render_command->buffer());

      Group::doRender(renderer);
    }

    FenceScope fence(renderer->device->device, this->render_fence->fence);

    std::vector<VkSemaphore> wait_semaphores{};
    std::vector<VkSemaphore> signal_semaphores = { this->rendering_finished->semaphore };

    this->render_command->submit(this->render_queue,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                 this->render_fence->fence);
  }

  std::vector<VkAttachmentDescription> attachments;
  std::vector<std::shared_ptr<SubpassObject>> subpasses;
  std::vector<VkSubpassDescription> subpass_descriptions;
  VkQueue render_queue{ nullptr };
  std::unique_ptr<VulkanSemaphore> rendering_finished;
  std::unique_ptr<VulkanCommandBuffers> render_command;  
  std::shared_ptr<VulkanFence> render_fence;
  std::shared_ptr<VulkanRenderpass> renderpass;
};

class SwapchainObject : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(SwapchainObject)
  virtual ~SwapchainObject() = default;

  SwapchainObject(std::shared_ptr<FramebufferAttachment> color_attachment,
                  VkSurfaceKHR surface,
                  VkSurfaceFormatKHR surface_format,
                  VkPresentModeKHR present_mode) :
      color_attachment(std::move(color_attachment)),
      surface(surface),
      surface_format(surface_format),
      present_mode(present_mode)
  {}

private:
  void doAlloc(RenderManager * context) override
  {
    this->present_queue = context->device->getQueue(0, this->surface);
    this->swapchain_image_ready = std::make_unique<VulkanSemaphore>(context->device);
    this->swap_buffers_finished = std::make_unique<VulkanSemaphore>(context->device);
  }

  void doResize(RenderManager * context) override
  {
    this->doStage(context);
  }

  void doStage(RenderManager * context) override
  {
    VkSwapchainKHR prevswapchain = (this->swapchain) ? this->swapchain->swapchain : nullptr;

    this->swapchain = std::make_unique<VulkanSwapchain>(
      context->device,
      context->vulkan,
      this->surface,
      3,
      this->surface_format.format,
      this->surface_format.colorSpace,
      context->extent,
      1,
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      std::vector<uint32_t>{ 0, },
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      this->present_mode,
      VK_FALSE,
      prevswapchain);

    uint32_t count;
    THROW_ON_ERROR(context->vulkan->vkGetSwapchainImages(context->device->device, 
                                                        this->swapchain->swapchain, 
                                                        &count, 
                                                        nullptr));
    this->swapchain_images.resize(count);
    THROW_ON_ERROR(context->vulkan->vkGetSwapchainImages(context->device->device, 
                                                        this->swapchain->swapchain, 
                                                        &count, 
                                                        this->swapchain_images.data()));

    std::vector<VkImageMemoryBarrier> image_barriers(count);
    for (uint32_t i = 0; i < count; i++) {
      image_barriers[i] = {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER, // sType
        nullptr,                                // pNext
        0,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        0,                                       // srcQueueFamilyIndex
        0,                                       // dstQueueFamilyIndex
        this->swapchain_images[i],               // image
        this->subresource_range
      };
    }

    vkCmdPipelineBarrier(context->command->buffer(),
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr,
                         count, image_barriers.data());
  }

  void doRecord(RenderManager * recorder) override
  {
    this->swap_buffers_command = std::make_unique<VulkanCommandBuffers>(recorder->device, 
                                                                        this->swapchain_images.size(),
                                                                        VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    for (size_t i = 0; i < this->swapchain_images.size(); i++) {
      VkImageMemoryBarrier src_image_barriers[2] = { 
      {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // newLayout
        0,                                                             // srcQueueFamilyIndex
        0,                                                             // dstQueueFamilyIndex
        this->swapchain_images[i],                                     // image
        this->subresource_range,                                       // subresourceRange
      },{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
        VK_ACCESS_TRANSFER_READ_BIT,                                   // dstAccessMask
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                      // oldLayout
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // newLayout
        0,                                                             // srcQueueFamilyIndex
        0,                                                             // dstQueueFamilyIndex
        this->color_attachment->image->image,                          // image
        this->subresource_range,                                       // subresourceRange
      } };

      VulkanCommandBufferScope command_scope(this->swap_buffers_command->buffer(i));

      vkCmdPipelineBarrier(this->swap_buffers_command->buffer(i),
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 0, nullptr,
                           2, src_image_barriers);

      const VkImageSubresourceLayers subresource_layers{
        this->subresource_range.aspectMask,     // aspectMask
        this->subresource_range.baseMipLevel,   // mipLevel
        this->subresource_range.baseArrayLayer, // baseArrayLayer
        this->subresource_range.layerCount      // layerCount;
      };

      const VkOffset3D offset = {
        0, 0, 0
      };

      VkExtent3D extent3d = { recorder->extent.width, recorder->extent.height, 1 };

      VkImageCopy image_copy{
        subresource_layers,             // srcSubresource
        offset,                         // srcOffset
        subresource_layers,             // dstSubresource
        offset,                         // dstOffset
        extent3d                        // extent
      };

      vkCmdCopyImage(this->swap_buffers_command->buffer(i),
                     color_attachment->image->image,
                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                     this->swapchain_images[i],
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     1, &image_copy);

      VkImageMemoryBarrier dst_image_barriers[2] = { 
      {
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_TRANSFER_WRITE_BIT,                                  // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // oldLayout
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,                               // newLayout
        0,                                                             // srcQueueFamilyIndex
        0,                                                             // dstQueueFamilyIndex
        this->swapchain_images[i],                                     // image
        this->subresource_range,                                       // subresourceRange
      },{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
        nullptr,                                                       // pNext
        VK_ACCESS_TRANSFER_READ_BIT,                                   // srcAccessMask
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // dstAccessMask
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // oldLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                      // newLayout
        0,                                                             // srcQueueFamilyIndex
        0,                                                             // dstQueueFamilyIndex
        this->color_attachment->image->image,                          // image
        this->subresource_range,                                       // subresourceRange
      } };

      vkCmdPipelineBarrier(this->swap_buffers_command->buffer(i),
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           0, 0, nullptr, 0, nullptr,
                           2, dst_image_barriers);
    }
  }

  void doPresent(RenderManager * context) override
  {
    THROW_ON_ERROR(context->vulkan->vkAcquireNextImage(context->device->device,
      this->swapchain->swapchain,
      UINT64_MAX,
      this->swapchain_image_ready->semaphore,
      nullptr,
      &this->image_index));

    std::vector<VkSemaphore> wait_semaphores = { this->swapchain_image_ready->semaphore };
    std::vector<VkSemaphore> signal_semaphores = { this->swap_buffers_finished->semaphore };

    this->swap_buffers_command->submit(this->present_queue,
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      this->image_index,
      wait_semaphores,
      signal_semaphores);

    VkPresentInfoKHR present_info{
      VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,              // sType
      nullptr,                                         // pNext
      static_cast<uint32_t>(signal_semaphores.size()), // waitSemaphoreCount
      signal_semaphores.data(),                        // pWaitSemaphores
      1,                                               // swapchainCount
      &this->swapchain->swapchain,                     // pSwapchains
      &this->image_index,                              // pImageIndices
      nullptr                                          // pResults
    };

    THROW_ON_ERROR(context->vulkan->vkQueuePresent(this->present_queue, &present_info));
  }

  std::shared_ptr<FramebufferAttachment> color_attachment;
  VkSurfaceKHR surface;
  VkPresentModeKHR present_mode;
  VkSurfaceFormatKHR surface_format;
  VkQueue present_queue;

  std::unique_ptr<VulkanSwapchain> swapchain;
  std::vector<VkImage> swapchain_images;
  std::unique_ptr<VulkanCommandBuffers> swap_buffers_command;
  std::unique_ptr<VulkanSemaphore> swapchain_image_ready;
  std::unique_ptr<VulkanSemaphore> swap_buffers_finished;

  const VkImageSubresourceRange subresource_range{
    VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
  };

  uint32_t image_index{ 0 };
};


class OffscreenImage : public Node {
public:
	NO_COPY_OR_ASSIGNMENT(OffscreenImage)
	virtual ~OffscreenImage() = default;

  OffscreenImage(std::shared_ptr<FramebufferAttachment> color_attachment)
		: color_attachment(color_attachment)
	{}

private:
	void doAlloc(RenderManager* context) override
	{
    this->offscreen_fence = std::make_unique<VulkanFence>(context->device);

    VkExtent3D extent = { context->extent.width, context->extent.height, 1 };

    this->image = std::make_shared<VulkanImage>(context->device,
                                                VK_IMAGE_TYPE_2D,
                                                VK_FORMAT_R8G8B8A8_UNORM,
                                                extent,
                                                this->subresource_range.levelCount,
                                                this->subresource_range.layerCount,
                                                VK_SAMPLE_COUNT_1_BIT,
                                                VK_IMAGE_TILING_LINEAR,
                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                VK_SHARING_MODE_EXCLUSIVE);

    this->image_object = std::make_shared<ImageObject>(this->image, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    context->imageobjects.push_back(this->image_object);
	}

  void doResize(RenderManager* context) override
  {
    this->doAlloc(context);
  }

	void doRecord(RenderManager* recorder) override
	{
		this->get_image_command = std::make_unique<VulkanCommandBuffers>(recorder->device);
		VulkanCommandBufferScope command_scope(this->get_image_command->buffer());

    VkImageMemoryBarrier src_image_barriers[2] = { 
    {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
      nullptr,                                                       // pNext
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
      VK_ACCESS_TRANSFER_READ_BIT,                                   // dstAccessMask
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                      // oldLayout
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // newLayout
      0,                                                             // srcQueueFamilyIndex
      0,                                                             // dstQueueFamilyIndex
      this->color_attachment->image->image,                          // image
      this->subresource_range,                                       // subresourceRange
    }, {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
      nullptr,                                                       // pNext
      0,                                                             // srcAccessMask
      VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
      VK_IMAGE_LAYOUT_UNDEFINED,                                     // oldLayout
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // newLayout
      0,                                                             // srcQueueFamilyIndex
      0,                                                             // dstQueueFamilyIndex
      this->image->image,                                            // image
      this->subresource_range,                                       // subresourceRange
      }
    };

		vkCmdPipelineBarrier(this->get_image_command->buffer(),
 			                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			                   0, 0, nullptr, 0, nullptr,
			                   2, src_image_barriers);

    const VkImageSubresourceLayers subresource_layers{
      this->subresource_range.aspectMask,     // aspectMask
      this->subresource_range.baseMipLevel,   // mipLevel
      this->subresource_range.baseArrayLayer, // baseArrayLayer
      this->subresource_range.layerCount      // layerCount;
    };

    const VkOffset3D offset = {
      0, 0, 0
    };

    VkExtent3D extent3d = { recorder->extent.width, recorder->extent.height, 1 };

    VkImageCopy image_copy{
      subresource_layers,             // srcSubresource
      offset,                         // srcOffset
      subresource_layers,             // dstSubresource
      offset,                         // dstOffset
      extent3d                        // extent
    };

    vkCmdCopyImage(this->get_image_command->buffer(),
                   this->color_attachment->image->image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   this->image->image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &image_copy);


    VkImageMemoryBarrier dst_image_barriers[2] = {
    {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
      nullptr,                                                       // pNext
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,                          // srcAccessMask
      VK_ACCESS_TRANSFER_READ_BIT,                                   // dstAccessMask
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,                          // oldLayout
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,                      // newLayout
      0,                                                             // srcQueueFamilyIndex
      0,                                                             // dstQueueFamilyIndex
      this->color_attachment->image->image,                          // image
      this->subresource_range,                                       // subresourceRange
    }, {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,                        // sType
      nullptr,                                                       // pNext
      0,                                                             // srcAccessMask
      VK_ACCESS_TRANSFER_WRITE_BIT,                                  // dstAccessMask
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,                          // oldLayout
      VK_IMAGE_LAYOUT_GENERAL,                                       // newLayout
      0,                                                             // srcQueueFamilyIndex
      0,                                                             // dstQueueFamilyIndex
      this->image->image,                                            // image
      this->subresource_range,                                       // subresourceRange
      }
    };

    vkCmdPipelineBarrier(this->get_image_command->buffer(),
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr,
                         2, dst_image_barriers);
	}

  void doPresent(RenderManager* context) override
  {
    {
      FenceScope fence_scope(context->device->device, this->offscreen_fence->fence);

      this->get_image_command->submit(context->queue,
                                      VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                                      this->offscreen_fence->fence);
    }

    VkImageSubresource image_subresource{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0 };
    VkSubresourceLayout subresource_layout;
    vkGetImageSubresourceLayout(context->device->device, this->image->image, &image_subresource, &subresource_layout);

    // Map image memory so we can start copying from it
    const char* data;
    vkMapMemory(context->device->device, this->image_object->memory->memory, 0, VK_WHOLE_SIZE, 0, (void**)& data);
    data += subresource_layout.offset;

    std::ofstream file("test.ppm", std::ios::out | std::ios::binary);

    // ppm header
    file << "P6\n" << context->extent.width << "\n" << context->extent.height << "\n" << 255 << "\n";

    // If source is BGR (destination is always RGB) and we can't use blit (which does automatic conversion), we'll have to manually swizzle color components
    bool colorSwizzle = false;
    // Check if source is BGR 
    // Note: Not complete, only contains most common and basic BGR surface formats for demonstation purposes
    std::vector<VkFormat> formatsBGR = { VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8A8_SNORM };
    colorSwizzle = (std::find(formatsBGR.begin(), formatsBGR.end(), this->color_attachment->format) != formatsBGR.end());

    // ppm binary pixel data
    for (uint32_t y = 0; y < context->extent.height; y++) {
      unsigned int* row = (unsigned int*)data;
      for (uint32_t x = 0; x < context->extent.width; x++) {
        if (colorSwizzle) {
          file.write((char*)row + 2, 1);
          file.write((char*)row + 1, 1);
          file.write((char*)row + 0, 1);
        }
        else {
          file.write((char*)row, 3);
        }
        row++;
      }
      data += subresource_layout.rowPitch;
    }
    file.close();
    vkUnmapMemory(context->device->device, this->image_object->memory->memory);

    std::cout << "Screenshot saved to disk" << std::endl;
  }

	std::shared_ptr<FramebufferAttachment> color_attachment;
	std::unique_ptr<VulkanCommandBuffers> get_image_command;
  const VkImageSubresourceRange subresource_range{
      VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
  };
  std::shared_ptr<VulkanImage> image;
  std::shared_ptr<ImageObject> image_object;
  std::shared_ptr<VulkanFence> offscreen_fence;
};
