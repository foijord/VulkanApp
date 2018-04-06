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

  void stage(class RenderAction * action)
  {
    this->doStage(action);
  }

  void record(class RenderAction * action)
  {
    this->doRecord(action);
  }

  void render(class RenderState & state)
  {
    this->doRender(state);
  }

private:
  virtual void doAlloc(class MemoryAllocator *) {}
  virtual void doStage(class RenderAction *) {}
  virtual void doRecord(class RenderAction *) {}
  virtual void doRender(class RenderState &) {}
};

