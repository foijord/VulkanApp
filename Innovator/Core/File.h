#pragma once

#include <Innovator/Nodes.h>
#include <Innovator/Core/Scheme/Scheme.h>

#include <string>
#include <memory>
#include <fstream>

class NodeExpression : public Expression {
public:
  NodeExpression(std::shared_ptr<Node> node)
    : node(node)
  {}
  std::shared_ptr<Node> node;
};

class LayoutBindingFunction : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    this->checkNumArgs(args, 3);
    Expression::iterator it = args->begin();
    auto bindingexpr = this->getNumber(*it++);
    auto typeexpr = this->getNumber(*it++);
    auto stageexpr = this->getNumber(*it++);

    uint32_t binding = static_cast<uint32_t>(bindingexpr->value);
    VkDescriptorType type = static_cast<VkDescriptorType>(int(typeexpr->value));
    VkShaderStageFlags stage = static_cast<VkShaderStageFlags>(int(stageexpr->value));

    auto node = std::make_shared<LayoutBinding>(binding, type, stage);
    return std::make_shared<NodeExpression>(node);
  }
};

class SamplerFunction : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    this->checkNumArgs(args, 0);
    auto node = std::make_shared<Sampler>();
    return std::make_shared<NodeExpression>(node);
  }
};

class TextureFunction : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    this->checkNumArgs(args, 1);
    auto filename = this->getString(args->front());
    auto node = std::make_shared<Texture>(filename->value);
    return std::make_shared<NodeExpression>(node);
  }
};

class ShaderFunction : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    this->checkNumArgs(args, 2);
    auto stage = this->getNumber(args->back());
    auto filename = this->getString(args->front());

    auto node = std::make_shared<Shader>(
      filename->value, 
      static_cast<VkShaderStageFlagBits>(int(stage->value)));

    return std::make_shared<NodeExpression>(node);
  }
};

class BoxFunction : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    this->checkNumArgs(args, 2);
    Expression::iterator it = args->begin();
    auto bindingexpr = this->getNumber(*it++);
    auto locationexpr = this->getNumber(*it++);

    auto node = std::make_shared<Box>(
      static_cast<uint32_t>(bindingexpr->value),
      static_cast<uint32_t>(locationexpr->value));

    return std::make_shared<NodeExpression>(node);
  }
};

class SeparatorFunction : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    auto sep = std::make_shared<Separator>();

    for (const std::shared_ptr<Expression> & exp : *args) {
      auto nodefunc = std::dynamic_pointer_cast<NodeExpression>(exp);
      if (!nodefunc) {
        throw std::invalid_argument("separator args must be nodes");
      }
      auto node = std::dynamic_pointer_cast<Node>(nodefunc->node);
      if (!node) {
        throw std::invalid_argument("separator args must be nodes");
      }
      sep->children.push_back(node);
    }
    return std::make_shared<NodeExpression>(sep);
  }
};

class File {
public:
  File() 
  {
    (*scheme.environment)["separator"] = std::make_shared<SeparatorFunction>();
    (*scheme.environment)["shader"] = std::make_shared<ShaderFunction>();
    (*scheme.environment)["VK_SHADER_STAGE_VERTEX_BIT"] = std::make_shared<Number>(VK_SHADER_STAGE_VERTEX_BIT);
    (*scheme.environment)["VK_SHADER_STAGE_COMPUTE_BIT"] = std::make_shared<Number>(VK_SHADER_STAGE_COMPUTE_BIT);
    (*scheme.environment)["VK_SHADER_STAGE_FRAGMENT_BIT"] = std::make_shared<Number>(VK_SHADER_STAGE_FRAGMENT_BIT);

    (*scheme.environment)["layout-binding"] = std::make_shared<LayoutBindingFunction>();
    (*scheme.environment)["VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER"] = std::make_shared<Number>(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    (*scheme.environment)["texture"] = std::make_shared<TextureFunction>();
    (*scheme.environment)["sampler"] = std::make_shared<SamplerFunction>();
    (*scheme.environment)["box"] = std::make_shared<BoxFunction>();
  }

  std::shared_ptr<Separator> open(const std::string & filename)
  {
    std::ifstream input(filename, std::ios::in);
    std::string code((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));

    auto sep = std::dynamic_pointer_cast<NodeExpression>(scheme.eval(code));
    if (!sep) {
      throw std::invalid_argument("top level expression must be a Node");
    }
    auto separator = std::dynamic_pointer_cast<Separator>(sep->node);
    if (!separator) {
      throw std::invalid_argument("top level node must be a Separator");
    }
    return separator;
  }

  Scheme scheme;
};