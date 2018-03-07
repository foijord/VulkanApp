#pragma once

class Node {
public:
  Node() = default;
  Node(const Node&) = delete;
  Node(const Node&&) = delete;
  Node & operator=(const Node&) = delete;
  Node & operator=(const Node&&) = delete;

  virtual ~Node() = default;
  virtual void traverse(class RenderAction *) {}
  virtual void traverse(class BoundingBoxAction *) {}
  virtual void traverse(class HandleEventAction *) {}
};

