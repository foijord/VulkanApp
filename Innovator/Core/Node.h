#pragma once

#include <Innovator/Core/Misc/Defines.h>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node)

  Node() = default;
  virtual ~Node() = default;

  void alloc(class MemoryAllocator * allocator)
  {
    this->doAlloc(allocator);
  }

  void stage(class MemoryStager * stager)
  {
    this->doStage(stager);
  }

  void record(class SceneManager * action)
  {
    this->doRecord(action);
  }

  void render(class SceneRenderer * renderer)
  {
    this->doRender(renderer);
  }

private:
  virtual void doAlloc(class MemoryAllocator *) {}
  virtual void doStage(class MemoryStager *) {}
  virtual void doRecord(class SceneManager *) {}
  virtual void doRender(class SceneRenderer *) {}
};

class Group : public Node {
public:
  NO_COPY_OR_ASSIGNMENT(Group)
  Group() = default;
  virtual ~Group() = default;

  explicit Group(std::vector<std::shared_ptr<Node>> children)
    : children(std::move(children)) {}

  std::vector<std::shared_ptr<Node>> children;

protected:
  void doAlloc(MemoryAllocator * allocator) override
  {
    for (const auto& node : this->children) {
      node->alloc(allocator);
    }
  }

  void doStage(MemoryStager * stager) override
  {
    for (const auto& node : this->children) {
      node->stage(stager);
    }
  }

  void doRecord(SceneManager * action) override
  {
    for (const auto& node : this->children) {
      node->record(action);
    }
  }

  void doRender(class SceneRenderer * renderer) override
  {
    for (const auto& node : this->children) {
      node->render(renderer);
    }
  }
};
