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

class NodeExpression : public Expression {
public:
  explicit NodeExpression(std::shared_ptr<Node> node)
    : node(std::move(node))
  {}
  std::shared_ptr<Node> node;
};

class SamplerFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 0);
    return std::make_shared<NodeExpression>(std::make_shared<Sampler>());
  }
};

template <typename T>
class InlineBufferDataFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args);
    auto bufferdata = std::make_shared<InlineBufferData<T>>(get_values<T>(args));
    return std::make_shared<NodeExpression>(bufferdata);
  }
};

template <typename T>
class STLBufferDataFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    const auto filename = get_string(args, 0);
    const auto bufferdata = std::make_shared<STLBufferData>(filename->value);
    return std::make_shared<NodeExpression>(bufferdata);
  }
};

template <typename T>
class MemoryBufferFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args);
    uint32_t flags = 0;
    for (const auto& it : args->children) {
      const auto number = std::dynamic_pointer_cast<Number>(it);
      if (!number) {
        throw std::invalid_argument("parameter must be a number");
      }
      flags |= static_cast<uint32_t>(number->value);
    }
    return std::make_shared<NodeExpression>(std::make_shared<T>(flags, 0));
  }
};

typedef MemoryBufferFunction<CpuMemoryBuffer> CpuMemoryBufferFunction;
typedef MemoryBufferFunction<GpuMemoryBuffer> GpuMemoryBufferFunction;

class VertexInputAttributeDescriptionFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 4);
    const auto location = get_number(args, 0);
    const auto binding = get_number(args, 1);
    const auto format = get_number(args, 2);
    const auto offset = get_number(args, 3);

    return std::make_shared<NodeExpression>(std::make_shared<VertexInputAttributeDescription>(
      static_cast<uint32_t>(location->value),
      static_cast<uint32_t>(binding->value),
      static_cast<VkFormat>(uint32_t(format->value)),
      static_cast<uint32_t>(offset->value)));
  }
};

class VertexInputBindingDescriptionFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 3);
    const auto binding = get_number(args, 0);
    const auto stride = get_number(args, 1);
    const auto inputRate = get_number(args, 2);

    return std::make_shared<NodeExpression>(std::make_shared<VertexInputBindingDescription>(
      static_cast<uint32_t>(binding->value),
      static_cast<uint32_t>(stride->value),
      static_cast<VkVertexInputRate>(uint32_t(inputRate->value))));
  }
};

class DrawCommandFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 2);
    const auto stride = get_number(args, 0);
    const auto topology = get_number(args, 1);

    return std::make_shared<NodeExpression>(std::make_shared<DrawCommand>(
      static_cast<VkPrimitiveTopology>(uint32_t(topology->value)),
      static_cast<VkDeviceSize>(stride->value)));
  }
};

class IndexedDrawCommandFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    const auto topology = get_number(args, 0);

    return std::make_shared<NodeExpression>(std::make_shared<IndexedDrawCommand>(
      static_cast<VkPrimitiveTopology>(uint32_t(topology->value))));
  }
};

class ImageFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    const auto filename = get_string(args, 0);
    return std::make_shared<NodeExpression>(std::make_shared<Image>(filename->value));
  }
};

class ShaderStageFlagBits : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(ShaderStageFlagBits)
  ShaderStageFlagBits() = delete;
  virtual ~ShaderStageFlagBits() = default;

  explicit ShaderStageFlagBits(const VkShaderStageFlagBits value) 
    : value(value) {}

  VkShaderStageFlagBits value;
};

class ShaderStageFlags : public Expression {
public:
  NO_COPY_OR_ASSIGNMENT(ShaderStageFlags)
  ShaderStageFlags() = delete;
  virtual ~ShaderStageFlags() = default;

  explicit ShaderStageFlags(const VkShaderStageFlags value) 
    : value(value) {}

  VkShaderStageFlags value;
};

Environment shader_stage_flag_bits {
  { "VK_SHADER_STAGE_VERTEX_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_VERTEX_BIT) },
  { "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) },
  { "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) },
  { "VK_SHADER_STAGE_GEOMETRY_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_GEOMETRY_BIT) },
  { "VK_SHADER_STAGE_FRAGMENT_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_FRAGMENT_BIT) },
  { "VK_SHADER_STAGE_COMPUTE_BIT", std::make_shared<ShaderStageFlagBits>(VK_SHADER_STAGE_COMPUTE_BIT) }
};

class ShaderStageFlagBitsFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    const auto flag = std::dynamic_pointer_cast<ShaderStageFlagBits>(args->children.front());
    if (!flag) {
      throw std::invalid_argument("shader-stage: argument must be shader stage flag");
    }
    return std::make_shared<ShaderStageFlagBits>(flag->value);
  }
};

class ShaderStageFlagsFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    // TODO: or together multiple arguments
    check_num_args(args, 1);
    const auto flag = std::dynamic_pointer_cast<ShaderStageFlagBits>(args->children.front());
    if (!flag) {
      throw std::invalid_argument("shader-stage: argument must be shader stage flag");
    }
    return std::make_shared<ShaderStageFlags>(flag->value);
  }
};

class LayoutBindingFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 3);
    const auto bindingexpr = get_number(args, 0);
    const auto typeexpr = get_number(args, 1);

    const auto it = std::next(args->children.begin(), 2);
    const auto stage_flags = std::dynamic_pointer_cast<ShaderStageFlags>(*it);
    if (!stage_flags) {
      throw std::invalid_argument("parameter must be one or more shader stage flags");
    }

    auto binding = static_cast<uint32_t>(bindingexpr->value);
    auto type = static_cast<VkDescriptorType>(int(typeexpr->value));
    auto stage = stage_flags->value;

    return std::make_shared<NodeExpression>(std::make_shared<DescriptorSetLayoutBinding>(binding, type, stage));
  }
};

class ShaderFunction : public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 2);

    const auto it = std::next(args->children.begin(), 1);
    const auto stage = std::dynamic_pointer_cast<ShaderStageFlagBits>(*it);

    const auto filename = get_string(args, 0);

    auto node = std::make_shared<Shader>(
      filename->value, 
      stage->value);

    return std::make_shared<NodeExpression>(node);
  }
};

class IndexBufferDescriptionFunction: public Callable {
public:
  exp_ptr operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    const auto type = get_number(args, 0);
    return std::make_shared<NodeExpression>(
      std::make_shared<IndexBufferDescription>(static_cast<VkIndexType>(uint32_t(type->value))));
  }
};

class TransformBufferFunction : public Callable {
  exp_ptr operator()(const Expression *) const override
  {
    auto node = std::make_shared<TransformBuffer>();
    return std::make_shared<NodeExpression>(node);
  }
};

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

class SeparatorFunction : public Callable {
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
    Environment node_env {
      { "separator", std::make_shared<SeparatorFunction>() },
      { "shader-stage-flag-bits", std::make_shared<ShaderStageFlagBitsFunction>() },
      { "shader-stage-flags", std::make_shared<ShaderStageFlagsFunction>() },
      { "shader", std::make_shared<ShaderFunction>() },
      { "layout-binding", std::make_shared<LayoutBindingFunction>() },
      { "sampler", std::make_shared<SamplerFunction>() },
      { "drawcommand", std::make_shared<DrawCommandFunction>() },
      { "indexeddrawcommand", std::make_shared<IndexedDrawCommandFunction>() },
      { "image", std::make_shared<ImageFunction>() },
      { "bufferdataui32", std::make_shared<InlineBufferDataFunction<uint32_t>>() },
      { "bufferdataf32", std::make_shared<InlineBufferDataFunction<float>>() },
      { "stlbufferdata", std::make_shared<STLBufferDataFunction<float>>() },
      { "cpumemorybuffer", std::make_shared<CpuMemoryBufferFunction>() },
      { "gpumemorybuffer", std::make_shared<GpuMemoryBufferFunction>() },
      { "transformbuffer", std::make_shared<TransformBufferFunction>() },
      { "indexbufferdescription", std::make_shared<IndexBufferDescriptionFunction>() },
      { "vertexinputattributedescription", std::make_shared<VertexInputAttributeDescriptionFunction>() },
      { "vertexinputbindingdescription", std::make_shared<VertexInputBindingDescriptionFunction>() },
      { "VK_INDEX_TYPE_UINT32", std::make_shared<Number>(VK_INDEX_TYPE_UINT32) },
      { "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER", std::make_shared<Number>(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) },
      { "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER", std::make_shared<Number>(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) },
      { "VK_BUFFER_USAGE_TRANSFER_SRC_BIT", std::make_shared<Number>(VK_BUFFER_USAGE_TRANSFER_SRC_BIT) },
      { "VK_BUFFER_USAGE_TRANSFER_DST_BIT", std::make_shared<Number>(VK_BUFFER_USAGE_TRANSFER_DST_BIT) },
      { "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT", std::make_shared<Number>(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT", std::make_shared<Number>(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_INDEX_BUFFER_BIT", std::make_shared<Number>(VK_BUFFER_USAGE_INDEX_BUFFER_BIT) },
      { "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT", std::make_shared<Number>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) },
      { "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST", std::make_shared<Number>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) },
      { "VK_VERTEX_INPUT_RATE_VERTEX", std::make_shared<Number>(VK_VERTEX_INPUT_RATE_VERTEX) },
      { "VK_FORMAT_R32G32B32_SFLOAT", std::make_shared<Number>(VK_FORMAT_R32G32B32_SFLOAT) },
    };

    node_env.insert(shader_stage_flag_bits.begin(), shader_stage_flag_bits.end());

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