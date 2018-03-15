#pragma once

#include <Innovator/Core/Misc/Defines.h>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node);

  Node() = default;
  virtual ~Node() = default;

  void traverse(class RenderAction * action) 
  {
    this->doAction(action);
  }

  void traverse(class BoundingBoxAction * action) 
  {
    this->doAction(action);
  }

  void traverse(class HandleEventAction * action) 
  {
    this->doAction(action);
  }

private:
  virtual void doAction(class RenderAction *) {}
  virtual void doAction(class BoundingBoxAction *) {}
  virtual void doAction(class HandleEventAction *) {}
};

