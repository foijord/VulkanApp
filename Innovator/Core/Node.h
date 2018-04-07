#pragma once

#include <Innovator/Core/Misc/Defines.h>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node);

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

