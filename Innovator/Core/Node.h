#pragma once

#include <Innovator/Core/Misc/Defines.h>

class Node {
public:
  NO_COPY_OR_ASSIGNMENT(Node);

  Node() = default;
  virtual ~Node() = default;

  virtual void traverse(class RenderAction *) {}
  virtual void traverse(class BoundingBoxAction *) {}
  virtual void traverse(class HandleEventAction *) {}
};

