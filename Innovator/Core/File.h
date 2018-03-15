#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/Core/Scheme/Scheme.h>

#include <string>
#include <memory>
#include <fstream>

class NodeExpression : public Expression {
public:
  explicit NodeExpression(std::shared_ptr<Node> node)
    : node(std::move(node))
  {}
  std::shared_ptr<Node> node;
};

class LayoutBindingFunction : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    check_num_args(args, 3);
    const auto bindingexpr = get_number(args, 0);
    const auto typeexpr = get_number(args, 1);
    const auto stageexpr = get_number(args, 2);

    auto binding = static_cast<uint32_t>(bindingexpr->value);
    auto type = static_cast<VkDescriptorType>(int(typeexpr->value));
    auto stage = static_cast<VkShaderStageFlags>(int(stageexpr->value));

    return std::make_shared<NodeExpression>(std::make_shared<LayoutBinding>(binding, type, stage));
  }
};

class SamplerFunction : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    check_num_args(args, 0);
    return std::make_shared<NodeExpression>(std::make_shared<Sampler>());
  }
};

class TextureFunction : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    check_num_args(args, 1);
    const auto filename = get_string(args, 0);
    return std::make_shared<NodeExpression>(std::make_shared<Texture>(filename->value));
  }
};

class ShaderFunction : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    check_num_args(args, 2);
    const auto stage = get_number(args, 1);
    const auto filename = get_string(args, 0);

    auto node = std::make_shared<Shader>(
      filename->value, 
      static_cast<VkShaderStageFlagBits>(int(stage->value)));

    return std::make_shared<NodeExpression>(node);
  }
};

class BoxFunction : public Callable {
public:
  std::shared_ptr<Expression> operator()(const Expression * args) const override
  {
    auto node = std::make_shared<Box>();
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
  std::shared_ptr<Expression> operator()(const Expression * args) const override
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
      { "shader", std::make_shared<ShaderFunction>() },
      { "VK_SHADER_STAGE_VERTEX_BIT", std::make_shared<Number>(VK_SHADER_STAGE_VERTEX_BIT) },
      { "VK_SHADER_STAGE_COMPUTE_BIT", std::make_shared<Number>(VK_SHADER_STAGE_COMPUTE_BIT) },
      { "VK_SHADER_STAGE_FRAGMENT_BIT", std::make_shared<Number>(VK_SHADER_STAGE_FRAGMENT_BIT) },
      { "layout-binding", std::make_shared<LayoutBindingFunction>() },
      { "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER", std::make_shared<Number>(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) },
      { "texture", std::make_shared<TextureFunction>() },
      { "sampler", std::make_shared<SamplerFunction>() },
      { "box", std::make_shared<BoxFunction>() },
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