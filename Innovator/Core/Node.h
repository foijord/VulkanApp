#pragma once

#include <Innovator/Core/Misc/Defines.h>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node);

  Node() = default;
  virtual ~Node() = default;

  void alloc(class RenderAction * action)
  {
    this->doAlloc(action);
  }

  void stage(class RenderAction * action)
  {
    this->doStage(action);
  }

  void record(class RenderAction * action)
  {
    this->doRecord(action);
  }

  void render(class RenderAction * action)
  {
    this->doRender(action);
  }

  void render(class BoundingBoxAction * action) 
  {
    this->doRender(action);
  }

  void render(class HandleEventAction * action) 
  {
    this->doRender(action);
  }

private:
  virtual void doAlloc(class RenderAction *) {}
  virtual void doStage(class RenderAction *) {}
  virtual void doRecord(class RenderAction *) {}
  virtual void doRender(class RenderAction *) {}
  virtual void doRender(class BoundingBoxAction *) {}
  virtual void doRender(class HandleEventAction *) {}
};

