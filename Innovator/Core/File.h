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

class ShaderFunction : public Callable {
public:
  virtual std::shared_ptr<Expression> operator()(const std::shared_ptr<Expression> & args)
  {
    this->checkNumArgs(args, 2);
    auto stage = this->getNumber(args->back());
    auto filename = this->getString(args->front());

    int value = static_cast<int>(stage->value);
    auto node = std::make_shared<Shader>(filename->value, static_cast<VkShaderStageFlagBits>(value));
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