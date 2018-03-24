#pragma once

#include <Innovator/Core/Misc/Defines.h>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node);

  Node() = default;
  virtual ~Node() = default;

  void render(class RenderAction * action) 
  {
    this->doRender(action);
  }

  void staging(class RenderAction * action)
  {
    this->doStaging(action);
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
  virtual void doStaging(class RenderAction *) {}
  virtual void doRender(class RenderAction *) {}
  virtual void doRender(class BoundingBoxAction *) {}
  virtual void doRender(class HandleEventAction *) {}
};

