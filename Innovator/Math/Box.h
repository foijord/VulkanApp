#pragma once

#include <glm/glm.hpp>
#include <limits>
#include <algorithm>

class box3 {
public:
  box3()
    : min(std::numeric_limits<float>::max()),
      max(-std::numeric_limits<float>::max())
  {}

  box3(glm::vec3 min, glm::vec3 max)
    : min(min), max(max) {}

  ~box3() {}

  void extendBy(const box3 & b)
  {
    this->extendBy(b.min);
    this->extendBy(b.max);
  }

  void extendBy(const glm::vec3 & v)
  {
    this->min = glm::min(v, this->min);
    this->max = glm::max(v, this->max);
  }

  void transform(const glm::mat4 & mat)
  {
    this->min = glm::vec3(mat * glm::vec4(this->min, 1.0));
    this->max = glm::vec3(mat * glm::vec4(this->max, 1.0));
  }

  float size() const
  {
    return glm::length(this->span());
  }

  glm::vec3 span() const
  {
    return this->max - this->min;
  }

  glm::vec3 center() const
  {
    return this->min + this->span() / 2.0f;
  }

  glm::vec3 min, max;
};