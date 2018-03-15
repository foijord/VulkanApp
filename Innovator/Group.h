#pragma once

#include <Innovator/Core/Node.h>

#define TRAVERSE_CHILDREN(self)             \
  for (const auto& node : self->children) { \
    node->traverse(action);                 \
  }                                         \

class Group : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Group);
  Group() = default;
  virtual ~Group() = default;

  explicit Group(std::vector<std::shared_ptr<Node>> children)
    : children(std::move(children)) {}

  std::vector<std::shared_ptr<Node>> children;

protected:
  void traverseChildren(RenderAction * action)
  {
    TRAVERSE_CHILDREN(this);
  }

  void traverseChildren(BoundingBoxAction * action)
  {
    TRAVERSE_CHILDREN(this);
  }

  void traverseChildren(HandleEventAction * action)
  {
    TRAVERSE_CHILDREN(this);
  }

private:
  void doAction(RenderAction * action) override
  {
    this->traverseChildren(action);
  }

  void doAction(BoundingBoxAction * action) override
  {
    this->traverseChildren(action);
  }

  void doAction(HandleEventAction * action) override
  {
    this->traverseChildren(action);
  }
};
