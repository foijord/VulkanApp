#pragma once

#include <Innovator/Core/Node.h>

template <typename NodeType, typename ActionType>
void traverse_children(NodeType * node, ActionType * action)
{
  for (const auto node : node->children) {
    node->traverse(action);
  }
}

class Group : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Group);
  Group() = default;
  virtual ~Group() = default;

  explicit Group(std::vector<std::shared_ptr<Node>> children)
    : children(std::move(children)) {}

  std::vector<std::shared_ptr<Node>> children;

private:
  void doAction(RenderAction * action) override
  {
    traverse_children(this, action);
  }

  void doAction(BoundingBoxAction * action) override
  {
    traverse_children(this, action);
  }

  void doAction(HandleEventAction * action) override
  {
    traverse_children(this, action);
  }
};
