#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/Math/Matrix.h>
#include <Innovator/Scheme/Scheme.h>

using namespace Innovator::Math;

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <iterator>

template <typename T>
class Value : public Expression {
public:
  explicit Value(const T value) : value(value) {}
  T value;
};

class LocationValue : public Expression {
public:
  explicit LocationValue(const uint32_t value) : value(value) {}
  uint32_t value;
};

class StrideValue : public Expression {
public:
  explicit StrideValue(const uint32_t value) : value(value) {}
  uint32_t value;
};

class BindingValue : public Expression {
public:
  explicit BindingValue(const uint32_t value) : value(value) {}
  uint32_t value;
};

class OffsetValue : public Expression {
public:
  explicit OffsetValue(const uint32_t value) : value(value) {}
  uint32_t value;
};

template <typename ValueType, typename ArgType>
class ValueFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    const auto flag = std::dynamic_pointer_cast<ArgType>(args->children.front());
    if (!flag) {
      throw std::invalid_argument("ValueFunction: argument must be ValueType");
    }
    return std::make_shared<ValueType>(flag->value);
  }
};

template <typename FlagsType, typename ValueType>
class FlagsFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args);
    uint32_t flags = 0;
    for (auto arg : args->children) {
      const auto flagbits = std::dynamic_pointer_cast<ValueType>(arg);
      if (!flagbits) {
        throw std::invalid_argument("invalid argument to FlagsFunction");
      }
      flags |= flagbits->value;
    }
    return std::make_shared<FlagsType>(flags);
  }
};

class NodeExpression : public Expression {
public:
  explicit NodeExpression(std::shared_ptr<Node> node)
    : node(std::move(node))
  {}
  std::shared_ptr<Node> node;
};

template <typename NodeType>
class NoArgsFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 0);
    return std::make_shared<NodeExpression>(std::make_shared<NodeType>());
  }
};

template <typename NodeType, typename ArgType>
class OneArgFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    return std::make_shared<NodeExpression>(
      std::make_shared<NodeType>(get_arg<ArgType>(args)->value));
  }
};

template <typename NodeType, typename ArgType0, typename ArgType1>
class TwoArgFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 2);
    return std::make_shared<NodeExpression>(
      std::make_shared<NodeType>(get_arg<ArgType0>(args)->value, 
                                 get_arg<ArgType1>(args)->value));
  }
};

template <typename NodeType, typename ArgType0, typename ArgType1, typename ArgType2>
class ThreeArgFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 3);
    return std::make_shared<NodeExpression>(
      std::make_shared<NodeType>(get_arg<ArgType0>(args)->value, 
                                 get_arg<ArgType1>(args)->value, 
                                 get_arg<ArgType2>(args)->value));
  }
};

template <typename NodeType, typename ArgType0, typename ArgType1, typename ArgType2, typename ArgType3>
class FourArgFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 4);
    return std::make_shared<NodeExpression>(
      std::make_shared<NodeType>(get_arg<ArgType0>(args)->value, 
                                 get_arg<ArgType1>(args)->value, 
                                 get_arg<ArgType2>(args)->value,
                                 get_arg<ArgType3>(args)->value));
  }
};

typedef ValueFunction<StrideValue, Number> StrideValueFunction;
typedef ValueFunction<OffsetValue, Number> OffsetValueFunction;
typedef ValueFunction<BindingValue, Number> BindingValueFunction;
typedef ValueFunction<LocationValue, Number> LocationValueFunction;

typedef Value<VkBufferUsageFlags> BufferUsageFlags;
typedef Value<VkBufferUsageFlagBits> BufferUsageFlagBits;
typedef FlagsFunction<BufferUsageFlags, BufferUsageFlagBits> BufferUsageFlagsFunction;

typedef Value<VkShaderStageFlags> ShaderStageFlags;
typedef Value<VkShaderStageFlagBits> ShaderStageFlagBits;
typedef ValueFunction<ShaderStageFlagBits, ShaderStageFlagBits> ShaderStageFlagBitsFunction;
typedef FlagsFunction<ShaderStageFlags, ShaderStageFlagBits> ShaderStageFlagsFunction;

typedef Value<VkDescriptorType> DescriptorType;
typedef Value<VkPrimitiveTopology> PrimitiveTopology;
typedef Value<VkVertexInputRate> VertexInputRate;
typedef Value<VkFormat> Format;
typedef Value<VkIndexType> IndexType;

typedef NoArgsFunction<Sampler> SamplerFunction;
typedef NoArgsFunction<TransformBuffer> TransformBufferFunction;
typedef OneArgFunction<STLBufferData, String> STLBufferDataFunction;
typedef OneArgFunction<DrawCommand, PrimitiveTopology> DrawCommandFunction;
typedef OneArgFunction<IndexedDrawCommand, PrimitiveTopology> IndexedDrawCommandFunction;
typedef OneArgFunction<Image, String> ImageFunction;
typedef OneArgFunction<CpuMemoryBuffer, BufferUsageFlags> CpuMemoryBufferFunction;
typedef OneArgFunction<GpuMemoryBuffer, BufferUsageFlags> GpuMemoryBufferFunction;
typedef OneArgFunction<IndexBufferDescription, IndexType> IndexBufferDescriptionFunction;
typedef TwoArgFunction<Shader, String, ShaderStageFlagBits> ShaderFunction;
typedef ThreeArgFunction<VertexInputBindingDescription, BindingValue, StrideValue, VertexInputRate> VertexInputBindingDescriptionFunction;
typedef ThreeArgFunction<DescriptorSetLayoutBinding, BindingValue, DescriptorType, ShaderStageFlags> DescriptorSetLayoutBindingFunction;
typedef FourArgFunction<VertexInputAttributeDescription, LocationValue, BindingValue, Format, OffsetValue> VertexInputAttributeDescriptionFunction;

template <typename T>
class InlineBufferDataFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args);
    return std::make_shared<NodeExpression>(
      std::make_shared<InlineBufferData<T>>(get_values<T>(args)));
  }
};

class SeparatorFunction : public Callable {
private:
  static std::shared_ptr<Node> extract_node(const exp_ptr exp)
  {
    const auto node_exp = std::dynamic_pointer_cast<NodeExpression>(exp);
    if (!node_exp) {
      throw std::invalid_argument("expression does not evaluate to a node");
    }
    const auto node = std::dynamic_pointer_cast<Node>(node_exp->node);
    if (!node) {
      throw std::logic_error("NodeExpression does not contain a node!");
    }
    return node;
  }
public:
  exp_ptr operator()(const Expression * args) const override
  {
    std::vector<std::shared_ptr<Node>> children(args->children.size());
    std::transform(args->children.begin(), args->children.end(), children.begin(), extract_node);

    auto sep = std::make_shared<Separator>(children);
    return std::make_shared<NodeExpression>(sep);
  }
};

class File {
public:
  File() 
  {
    static Environment node_env {
      { "stride", std::make_shared<StrideValueFunction>() },
      { "offset", std::make_shared<OffsetValueFunction>() },
      { "binding", std::make_shared<BindingValueFunction>() },
      { "location", std::make_shared<LocationValueFunction>() },
      // buffer usage flags
      { "buffer-usage-flags", std::make_shared<BufferUsageFlagsFunction>() },
      { "VK_BUFFER_USAGE_TRANSFER_SRC_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT) },
      { "VK_BUFFER_USAGE_TRANSFER_DST_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT) },
      { "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_INDEX_BUFFER_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT) },
      { "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT) },
      { "VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT) },
      { "VK_BUFFER_USAGE_RAY_TRACING_BIT_NV", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_RAY_TRACING_BIT_NV) },
      { "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT) },
      { "VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM", std::make_shared<BufferUsageFlagBits>(VK_BUFFER_USAGE_FLAG_BITS_MAX_ENUM) },
      // shader stage flags
      { "shader-stage-flags", std::make_shared<ShaderStageFlagsFunction>() },
      { "VK_SHADER_STAGE_VERTEX_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT) },
      { "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) },
      { "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) },
      { "VK_SHADER_STAGE_GEOMETRY_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_GEOMETRY_BIT) },
      { "VK_SHADER_STAGE_FRAGMENT_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_FRAGMENT_BIT) },
      { "VK_SHADER_STAGE_COMPUTE_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_COMPUTE_BIT) },
      // descriptor type
      { "VK_DESCRIPTOR_TYPE_SAMPLER", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_SAMPLER) },
      { "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) },
      { "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) },
      { "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE) },
      { "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) },
      { "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) },
      { "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) },
      { "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) },
      { "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) },
      { "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) },
      { "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) },
      { "VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT) },
      { "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV) },
      { "VK_DESCRIPTOR_TYPE_BEGIN_RANGE", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_BEGIN_RANGE) },
      { "VK_DESCRIPTOR_TYPE_END_RANGE", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_END_RANGE) },
      { "VK_DESCRIPTOR_TYPE_RANGE_SIZE", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_RANGE_SIZE) },
      { "VK_DESCRIPTOR_TYPE_MAX_ENUM", std::make_shared<DescriptorType>(VK_DESCRIPTOR_TYPE_MAX_ENUM) },
      // primitive topology
      { "VK_PRIMITIVE_TOPOLOGY_POINT_LIST", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_POINT_LIST) },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_LIST", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_LINE_LIST) },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP) },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP) },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN) },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY) },
      { "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY) },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY) },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY) },
      { "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) },
      { "VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_BEGIN_RANGE) },
      { "VK_PRIMITIVE_TOPOLOGY_END_RANGE", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_END_RANGE) },
      { "VK_PRIMITIVE_TOPOLOGY_RANGE_SIZE", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_RANGE_SIZE) },
      { "VK_PRIMITIVE_TOPOLOGY_MAX_ENUM", std::make_shared<PrimitiveTopology>(VK_PRIMITIVE_TOPOLOGY_MAX_ENUM) },
      // vertex input rate
      { "VK_VERTEX_INPUT_RATE_VERTEX", std::make_shared<VertexInputRate>(VK_VERTEX_INPUT_RATE_VERTEX) },
      { "VK_VERTEX_INPUT_RATE_INSTANCE", std::make_shared<VertexInputRate>(VK_VERTEX_INPUT_RATE_INSTANCE) },
      { "VK_VERTEX_INPUT_RATE_BEGIN_RANGE", std::make_shared<VertexInputRate>(VK_VERTEX_INPUT_RATE_BEGIN_RANGE) },
      { "VK_VERTEX_INPUT_RATE_END_RANGE", std::make_shared<VertexInputRate>(VK_VERTEX_INPUT_RATE_END_RANGE) },
      { "VK_VERTEX_INPUT_RATE_RANGE_SIZE", std::make_shared<VertexInputRate>(VK_VERTEX_INPUT_RATE_RANGE_SIZE) },
      { "VK_VERTEX_INPUT_RATE_MAX_ENUM", std::make_shared<VertexInputRate>(VK_VERTEX_INPUT_RATE_MAX_ENUM) },
      // format
      { "VK_FORMAT_R32_UINT", std::make_shared<Format>(VK_FORMAT_R32_UINT) },
      { "VK_FORMAT_R32_SINT", std::make_shared<Format>(VK_FORMAT_R32_SINT) },
      { "VK_FORMAT_R32_SFLOAT", std::make_shared<Format>(VK_FORMAT_R32_SFLOAT) },
      { "VK_FORMAT_R32G32_UINT", std::make_shared<Format>(VK_FORMAT_R32G32_UINT) },
      { "VK_FORMAT_R32G32_SINT", std::make_shared<Format>(VK_FORMAT_R32G32_SINT) },
      { "VK_FORMAT_R32G32_SFLOAT", std::make_shared<Format>(VK_FORMAT_R32G32_SFLOAT) },
      { "VK_FORMAT_R32G32B32_UINT", std::make_shared<Format>(VK_FORMAT_R32G32B32_UINT) },
      { "VK_FORMAT_R32G32B32_SINT", std::make_shared<Format>(VK_FORMAT_R32G32B32_SINT) },
      { "VK_FORMAT_R32G32B32_SFLOAT", std::make_shared<Format>(VK_FORMAT_R32G32B32_SFLOAT) },
      { "VK_FORMAT_R32G32B32A32_UINT", std::make_shared<Format>(VK_FORMAT_R32G32B32A32_UINT) },
      { "VK_FORMAT_R32G32B32A32_SINT", std::make_shared<Format>(VK_FORMAT_R32G32B32A32_SINT) },
      { "VK_FORMAT_R32G32B32A32_SFLOAT", std::make_shared<Format>(VK_FORMAT_R32G32B32A32_SFLOAT) },
      // index type
      { "VK_INDEX_TYPE_UINT16", std::make_shared<IndexType>(VK_INDEX_TYPE_UINT16) },
      { "VK_INDEX_TYPE_UINT32", std::make_shared<IndexType>(VK_INDEX_TYPE_UINT32) },
      { "VK_INDEX_TYPE_NONE_NV", std::make_shared<IndexType>(VK_INDEX_TYPE_NONE_NV) },
      { "VK_INDEX_TYPE_BEGIN_RANGE", std::make_shared<IndexType>(VK_INDEX_TYPE_BEGIN_RANGE) },
      { "VK_INDEX_TYPE_END_RANGE", std::make_shared<IndexType>(VK_INDEX_TYPE_END_RANGE) },
      { "VK_INDEX_TYPE_RANGE_SIZE", std::make_shared<IndexType>(VK_INDEX_TYPE_RANGE_SIZE) },
      { "VK_INDEX_TYPE_MAX_ENUM", std::make_shared<IndexType>(VK_INDEX_TYPE_MAX_ENUM) },
      // nodes
      { "separator", std::make_shared<SeparatorFunction>() },
      { "shader", std::make_shared<ShaderFunction>() },
      { "descriptor-set-layout-binding", std::make_shared<DescriptorSetLayoutBindingFunction>() },
      { "sampler", std::make_shared<SamplerFunction>() },
      { "drawcommand", std::make_shared<DrawCommandFunction>() },
      { "indexeddrawcommand", std::make_shared<IndexedDrawCommandFunction>() },
      { "image", std::make_shared<ImageFunction>() },
      { "bufferdataui32", std::make_shared<InlineBufferDataFunction<uint32_t>>() },
      { "bufferdataf32", std::make_shared<InlineBufferDataFunction<float>>() },
      { "stlbufferdata", std::make_shared<STLBufferDataFunction>() },
      { "cpumemorybuffer", std::make_shared<CpuMemoryBufferFunction>() },
      { "gpumemorybuffer", std::make_shared<GpuMemoryBufferFunction>() },
      { "transformbuffer", std::make_shared<TransformBufferFunction>() },
      { "indexbufferdescription", std::make_shared<IndexBufferDescriptionFunction>() },
      { "vertexinputattributedescription", std::make_shared<VertexInputAttributeDescriptionFunction>() },
      { "vertexinputbindingdescription", std::make_shared<VertexInputBindingDescriptionFunction>() }
    };
    this->scheme.environment->outer = std::make_shared<Environment>(node_env.begin(), node_env.end());
  }

  std::shared_ptr<Separator> open(const std::string & filename) const
  {
    std::ifstream input(filename, std::ios::in);
    const std::string code((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));

    const auto node_exp = std::dynamic_pointer_cast<NodeExpression>(scheme.eval(code));
    if (!node_exp) {
      throw std::invalid_argument("top level expression must be a Node");
    }
    auto separator = std::dynamic_pointer_cast<Separator>(node_exp->node);
    if (!separator) {
      throw std::invalid_argument("top level node must be a Separator");
    }
    return separator;
  }

  Scheme scheme;
};