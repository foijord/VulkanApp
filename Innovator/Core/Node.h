#pragma once

#include <Innovator/Core/Misc/Defines.h>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node);

  Node() = default;
  virtual ~Node() = default;

  void traverse(class RenderAction * action) 
  {
    this->do_traverse(action);
  }

  void traverse(class BoundingBoxAction * action) 
  {
    this->do_traverse(action);
  }

  void traverse(class HandleEventAction * action) 
  {
    this->do_traverse(action);
  }

private:
  virtual void do_traverse(class RenderAction *) = 0;
  virtual void do_traverse(class BoundingBoxAction *) {}
  virtual void do_traverse(class HandleEventAction *) {}
};

