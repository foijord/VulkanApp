#pragma once

#include <Innovator/Core/Node.h>

class Group : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Group);
  Group() = default;
  virtual ~Group() = default;

  explicit Group(std::vector<std::shared_ptr<Node>> children)
    : children(std::move(children)) {}

  void traverse(RenderAction * action) override
  {
    for (const auto& node : this->children) {
      node->traverse(action);
    }
  }

  void traverse(BoundingBoxAction * action) override
  {
    for (const auto& node : this->children) {
      node->traverse(action);
    }
  }

  void traverse(HandleEventAction * action) override
  {
    for (const auto& node : this->children) {
      node->traverse(action);
    }
  }

  std::vector<std::shared_ptr<Node>> children;
};
