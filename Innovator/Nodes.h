#pragma once

#include <Innovator/RenderManager.h>
#include <Innovator/Core/Node.h>
#include <Innovator/Core/Math/Matrix.h>
#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Misc/Factory.h>

#include <vulkan/vulkan.h>

#include <utility>
#include <vector>
#include <memory>

using namespace Innovator::Core::Math;

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

  void doPipeline(PipelineCreator * creator) override
  {
    StateScope<PipelineCreator, PipelineState> scope(creator);
    Group::doPipeline(creator);
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

class Transform : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Transform)
  Transform() = default;
  virtual ~Transform() = default;

  explicit Transform(const vec3d & t,
                     const vec3d & s)
  {
    for (size_t i = 0; i < 3; i++) {
      this->matrix.m[i].v[i] = s.v[i];
      this->matrix.m[3].v[i] = t.v[i];
    }
  }

private:
  void doRender(SceneRenderer * renderer) override
  {
    renderer->state.ModelMatrix = renderer->state.ModelMatrix * this->matrix;
  }

  mat4d matrix{
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
  };
};

template <typename T>
class BufferData : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(BufferData)
  BufferData() = delete;
  virtual ~BufferData() = default;

  explicit BufferData(std::vector<T> values) :
    values(std::move(values)),
    buffer_data_description({
      sizeof(T),                        // stride
      sizeof(T) * this->values.size(),  // size
      this->values.data(),              // data
    })
  {}

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    allocator->state.buffer_data_description = this->buffer_data_description;
  }

  void doStage(MemoryStager * stager) override
  {
    stager->state.buffer_data_description = this->buffer_data_description;
  }

  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.buffer_data_description = this->buffer_data_description;
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.buffer_data_description = this->buffer_data_description;
  }

  std::vector<T> values;
  VulkanBufferDataDescription buffer_data_description;
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
    state.descriptor_buffer_info = {
      this->buffer->buffer->buffer,
      this->buffer->offset,
      this->buffer->range,
    };
  }

  void doAlloc(MemoryAllocator * allocator) override
  {
    this->buffer = std::make_shared<BufferObject>(
      allocator->state.buffer_data_description.size,
      this->buffer_usage_flags,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    allocator->bufferobjects.push_back(this->buffer);
  }

  void doStage(MemoryStager * stager) override
  {
    this->updateState(stager->state);
    this->buffer->memcpy(stager->state.buffer_data_description.data);
  }

  void doPipeline(PipelineCreator * creator) override
  {
    this->updateState(creator->state);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.buffer = this->buffer->buffer->buffer;
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
      allocator->state.buffer_data_description.size,
      this->buffer_usage_flags,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    allocator->bufferobjects.push_back(this->buffer);
  }

  void doStage(MemoryStager * stager) override
  {
    std::vector<VkBufferCopy> regions = { {
        stager->state.descriptor_buffer_info.offset,     // srcOffset
        this->buffer->offset,                            // dstOffset
        stager->state.buffer_data_description.size,      // size
    } };

    vkCmdCopyBuffer(stager->command,
                    stager->state.descriptor_buffer_info.buffer,
                    this->buffer->buffer->buffer,
                    static_cast<uint32_t>(regions.size()),
                    regions.data());
  }

  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.descriptor_buffer_info = {
      this->buffer->buffer->buffer,
      this->buffer->offset,
      this->buffer->range,
    };
  }

  void doRecord(CommandRecorder * recorder) override
  {
    recorder->state.buffer = this->buffer->buffer->buffer;
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
      sizeof(Innovator::Core::Math::mat4f) * 2,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    allocator->bufferobjects.push_back(this->buffer);
  }

  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.descriptor_buffer_info = {
      this->buffer->buffer->buffer,
      this->buffer->offset,
      this->buffer->range,
    };
  }

  void doRender(SceneRenderer * renderer) override
  {
    mat4f data[] = {
      cast<float>(renderer->camera->viewmatrix() * renderer->state.ModelMatrix),
      cast<float>(renderer->camera->projmatrix())
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
    const auto count = recorder->state.buffer_data_description.size / recorder->state.buffer_data_description.stride;

    recorder->state.indices.push_back({
      this->type,
      recorder->state.buffer,
      static_cast<uint32_t>(count),
    });
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
  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.vertex_attributes.push_back(this->vertex_input_attribute_description);
  }

  void doRecord(CommandRecorder * recorder) override
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
  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.vertex_input_bindings.push_back({
      this->binding,
      static_cast<uint32_t>(creator->state.buffer_data_description.stride * this->stride),
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
    descriptor_image_info({
      nullptr, nullptr, VK_IMAGE_LAYOUT_UNDEFINED
    }),
    descriptor_buffer_info({
      nullptr, 0, 0
    }),
    descriptor_set_layout_binding({
      binding,
      descriptorType,
      1,
      stageFlags,
      nullptr,
    })
  {}

private:
  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.descriptor_pool_sizes.push_back({
      this->descriptor_set_layout_binding.descriptorType,                // type 
      this->descriptor_set_layout_binding.descriptorCount,               // descriptorCount
    });

    creator->state.descriptor_set_layout_bindings.push_back({
      this->descriptor_set_layout_binding
    });

    // copy these so they don't get overwritten!
    this->descriptor_image_info = creator->state.descriptor_image_info;
    this->descriptor_buffer_info = creator->state.descriptor_buffer_info;
   
    creator->state.write_descriptor_sets.push_back({
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,                           // sType
      nullptr,                                                          // pNext
      nullptr,                                                          // dstSet
      this->descriptor_set_layout_binding.binding,                      // dstBinding
      0,                                                                // dstArrayElement
      this->descriptor_set_layout_binding.descriptorCount,              // descriptorCount
      this->descriptor_set_layout_binding.descriptorType,               // descriptorType
      &this->descriptor_image_info,                                     // pImageInfo
      &this->descriptor_buffer_info,                                    // pBufferInfo
      nullptr,                                                          // pTexelBufferView
    });
  }

  VkDescriptorImageInfo descriptor_image_info{};
  VkDescriptorBufferInfo descriptor_buffer_info{};
  VkDescriptorSetLayoutBinding descriptor_set_layout_binding;
};

class Shader : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Shader)
  Shader() = delete;
  virtual ~Shader() = default;

  explicit Shader(std::string filename, const VkShaderStageFlagBits stage):
    filename(std::move(filename)), 
    stage(stage)
  {
    this->readFile();
  }

  void readFile() 
  {
    std::ifstream input(this->filename, std::ios::binary);
    this->code = std::vector<char>((std::istreambuf_iterator<char>(input)),
      (std::istreambuf_iterator<char>()));
  }

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    this->shader = std::make_unique<VulkanShaderModule>(allocator->device, this->code);
  }

  void doPipeline(PipelineCreator * creator) override
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
  std::string filename;
  std::vector<char> code;
  std::vector<uint32_t> module;
  VkShaderStageFlagBits stage;
  std::unique_ptr<VulkanShaderModule> shader;
};

class Sampler : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Sampler)
  Sampler() = default;
  virtual ~Sampler() = default;

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    this->sampler = std::make_unique<VulkanSampler>(
      allocator->device,
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
  }

  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.sampler = this->sampler->sampler;
  }

  std::unique_ptr<VulkanSampler> sampler;
};

class Image : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Image)

  explicit Image(std::shared_ptr<VulkanTextureImage> texture) :
    texture(std::move(texture)),
    sample_count(VK_SAMPLE_COUNT_1_BIT),
    sharing_mode(VK_SHARING_MODE_EXCLUSIVE),
    create_flags(0),
    usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    tiling(VK_IMAGE_TILING_OPTIMAL),
    component_mapping({
      VK_COMPONENT_SWIZZLE_R,         // r
      VK_COMPONENT_SWIZZLE_G,         // g
      VK_COMPONENT_SWIZZLE_B,         // b
      VK_COMPONENT_SWIZZLE_A}),       // a
    subresource_range({
      VK_IMAGE_ASPECT_COLOR_BIT,          // aspectMask 
      this->texture->base_level(),         // baseMipLevel 
      this->texture->levels(),             // levelCount 
      this->texture->base_layer(),         // baseArrayLayer 
      this->texture->layers()})            // layerCount 
  {}

  explicit Image(const std::string & filename) :
    Image(VulkanImageFactory::Create(filename))
  {}

  virtual ~Image() = default;

private:
  void doAlloc(MemoryAllocator * allocator) override
  {
    VkImageFormatProperties image_format_properties;

    THROW_ON_ERROR(vkGetPhysicalDeviceImageFormatProperties(
      allocator->device->physical_device.device,
      this->texture->format(),
      this->texture->image_type(),
      this->tiling,
      this->usage_flags,
      this->create_flags,
      &image_format_properties));

    this->buffer = std::make_shared<BufferObject>(
      this->texture->size(),
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    allocator->bufferobjects.push_back(this->buffer);

    this->image = std::make_shared<ImageObject>(
      std::make_shared<VulkanImage>(
        allocator->device,
        this->texture->image_type(),
        this->texture->format(),
        this->texture->extent(0),
        this->texture->levels(),
        this->texture->layers(),
        this->sample_count,
        this->tiling,
        this->usage_flags,
        this->sharing_mode,
        this->create_flags),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    allocator->imageobjects.push_back(this->image);
  }

  void doStage(MemoryStager * stager) override
  {
    this->buffer->memcpy(this->texture->data());

    this->view = std::make_unique<VulkanImageView>(
      this->image->image,
      this->texture->format(),
      this->texture->image_view_type(),
      this->component_mapping,
      this->subresource_range);

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
      this->subresource_range,                               // subresourceRange
    };

    vkCmdPipelineBarrier(stager->command,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0, 0, nullptr, 0, nullptr, 1,
                         &memory_barrier);

    std::vector<VkBufferImageCopy> regions;

    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < this->texture->levels(); mip_level++) {

      const VkImageSubresourceLayers subresource_layers{
        this->subresource_range.aspectMask,               // aspectMask
        mip_level,                                        // mipLevel
        this->subresource_range.baseArrayLayer,           // baseArrayLayer
        this->subresource_range.layerCount,               // layerCount
      };

      regions.push_back({
        buffer_offset,                             // bufferOffset 
        0,                                         // bufferRowLength
        0,                                         // bufferImageHeight
        subresource_layers,                        // imageSubresource
        { 0, 0, 0 },                               // imageOffset
        this->texture->extent(mip_level),           // imageExtent
      });

      buffer_offset += this->texture->size(mip_level);
    }

    vkCmdCopyBufferToImage(stager->command,
                           this->buffer->buffer->buffer,
                           this->image->image->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           static_cast<uint32_t>(regions.size()),
                           regions.data());
  }

  void doPipeline(PipelineCreator * creator) override
  {
    creator->state.descriptor_image_info = {
      creator->state.sampler,
      this->view->view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
  }

  std::shared_ptr<VulkanTextureImage> texture;

  std::shared_ptr<BufferObject> buffer;
  std::shared_ptr<ImageObject> image;
  std::unique_ptr<VulkanImageView> view;

  VkSampleCountFlagBits sample_count;
  VkSharingMode sharing_mode;
  VkImageCreateFlags create_flags;
  VkImageUsageFlags usage_flags;
  VkImageTiling tiling;
  VkComponentMapping component_mapping;
  VkImageSubresourceRange subresource_range;
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
  void doPipeline(PipelineCreator * creator) override
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
  void doPipeline(PipelineCreator * creator) override
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
      std::vector<VkDescriptorSetLayout>{ this->descriptor_set_layout->layout });

    for (auto & write_descriptor_set : creator->state.write_descriptor_sets) {
      write_descriptor_set.dstSet = this->descriptor_set->descriptor_sets[0];
    }
    this->descriptor_set->update(creator->state.write_descriptor_sets);

    this->pipeline = std::make_unique<VulkanComputePipeline>(
      creator->device,
      creator->pipelinecache,
      creator->state.shader_stage_infos[0],
      this->pipeline_layout->layout);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    vkCmdBindDescriptorSets(recorder->command,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            this->pipeline_layout->layout,
                            0,
                            static_cast<uint32_t>(this->descriptor_set->descriptor_sets.size()),
                            this->descriptor_set->descriptor_sets.data(),
                            0,
                            nullptr);

    vkCmdBindPipeline(recorder->command,
                      VK_PIPELINE_BIND_POINT_COMPUTE,
                      this->pipeline->pipeline);

    vkCmdDispatch(recorder->command,
                  this->group_count_x,
                  this->group_count_y,
                  this->group_count_z);
  }

  uint32_t group_count_x;
  uint32_t group_count_y;
  uint32_t group_count_z;

  std::unique_ptr<VulkanComputePipeline> pipeline;

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
  virtual void execute(VkCommandBuffer command, CommandRecorder * recorder) = 0;

  void doAlloc(MemoryAllocator * allocator) override
  {
    this->command = std::make_unique<VulkanCommandBuffers>(
      allocator->device,
      1,
      VK_COMMAND_BUFFER_LEVEL_SECONDARY);
  }

  void doPipeline(PipelineCreator * creator) override
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
      descriptor_set_layouts);

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
      creator->renderpass,
      creator->pipelinecache,
      this->pipeline_layout->layout,
      this->topology,
      creator->state.rasterization_state,
      this->dynamic_states,
      creator->state.shader_stage_infos,
      creator->state.vertex_input_bindings,
      creator->state.vertex_attributes);
  }

  void doRecord(CommandRecorder * recorder) override
  {
    VulkanCommandBufferScope command_scope(this->command->buffer(),
                                           recorder->renderpass,
                                           0,
                                           recorder->framebuffer,
                                           VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT);

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

  explicit DrawCommand(VkPrimitiveTopology topology, uint32_t count) :
    DrawCommandBase(topology),
    count(count)
  {}

private:
  void execute(VkCommandBuffer command, CommandRecorder *) override
  {
    vkCmdDraw(command, this->count, 1, 0, 0);
  }

  uint32_t count;  
};

class IndexedDrawCommand : public DrawCommandBase {
public:
  NO_COPY_OR_ASSIGNMENT(IndexedDrawCommand)
  virtual ~IndexedDrawCommand() = default;

  explicit IndexedDrawCommand(VkPrimitiveTopology topology) :
    DrawCommandBase(topology)
  {}

private:
  void execute(VkCommandBuffer command, CommandRecorder * recorder) override
  {
    for (const auto& indexbuffer : recorder->state.indices) {
      vkCmdBindIndexBuffer(command, indexbuffer.buffer, 0, indexbuffer.type);
      vkCmdDrawIndexed(command, indexbuffer.count, 1, 0, 0, 1);
    }
  }
};
