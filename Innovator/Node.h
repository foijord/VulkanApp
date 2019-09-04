#pragma once

#include <Innovator/Defines.h>
#include <vector>
#include <memory>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node)

  Node() = default;
  virtual ~Node() = default;

  void alloc(class RenderManager * context)
  {
    this->doAlloc(context);
  }

  void resize(class RenderManager * context)
  {
    this->doResize(context);
  }

  void stage(class RenderManager * context)
  {
    this->doStage(context);
  }

  void pipeline(class RenderManager * creator)
  {
    this->doPipeline(creator);
  }

  void record(class RenderManager * recorder)
  {
    this->doRecord(recorder);
  }

  void render(class SceneRenderer * renderer)
  {
    this->doRender(renderer);
  }

  void present(class RenderManager * context)
  {
    this->doPresent(context);
  }

private:
  virtual void doAlloc(class RenderManager *) {}
  virtual void doResize(class RenderManager *) {}
  virtual void doStage(class RenderManager *) {}
  virtual void doPipeline(class RenderManager *) {}
  virtual void doRecord(class RenderManager *) {}
  virtual void doRender(class SceneRenderer *) {}
  virtual void doPresent(class RenderManager *) {}
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
  void doAlloc(RenderManager * context) override
  {
    for (const auto& node : this->children) {
      node->alloc(context);
    }
  }

  void doResize(RenderManager * context) override
  {
    for (const auto& node : this->children) {
      node->resize(context);
    }
  }

  void doStage(RenderManager * context) override
  {
    for (const auto& node : this->children) {
      node->stage(context);
    }
  }

  void doPipeline(RenderManager * creator) override
  {
    for (const auto& node : this->children) {
      node->pipeline(creator);
    }
  }

  void doRecord(RenderManager * recorder) override
  {
    for (const auto& node : this->children) {
      node->record(recorder);
    }
  }

  void doRender(class SceneRenderer * renderer) override
  {
    for (const auto& node : this->children) {
      node->render(renderer);
    }
  }

  void doPresent(class RenderManager * context) override
  {
    for (const auto& node : this->children) {
      node->present(context);
    }
  }
};
