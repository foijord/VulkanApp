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
  void traverse_children(RenderAction * action)
  {
    TRAVERSE_CHILDREN(this);
  }

  void traverse_children(BoundingBoxAction * action)
  {
    TRAVERSE_CHILDREN(this);
  }

  void traverse_children(HandleEventAction * action)
  {
    TRAVERSE_CHILDREN(this);
  }

private:
  void do_traverse(RenderAction * action) override
  {
    this->traverse_children(action);
  }

  void do_traverse(BoundingBoxAction * action) override
  {
    this->traverse_children(action);
  }

  void do_traverse(HandleEventAction * action) override
  {
    this->traverse_children(action);
  }
};
