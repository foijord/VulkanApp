#pragma once

class Node {
public:
  virtual ~Node() {}
  virtual void traverse(class RenderAction *) {}
  virtual void traverse(class BoundingBoxAction *) {}
  virtual void traverse(class HandleEventAction *) {}
};

