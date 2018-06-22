#pragma once

#include <Innovator/Core/Math/Box.h>

#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
  NO_COPY_OR_ASSIGNMENT(Camera)
  ~Camera() = default;

  explicit Camera() :
    farplane(1000.0f),
    nearplane(0.1f),
    aspectratio(4.0f / 3.0f),
    fieldofview(0.7f),
    focaldistance(1.0f),
    position(glm::vec3(0, 0, 1)),
    orientation(glm::mat3(1.0))
  {}

  void zoom(float dy)
  {
    const auto focalpoint = this->position - this->orientation[2] * this->focaldistance;
    this->position += this->orientation[2] * dy;
    this->focaldistance = glm::length(this->position - focalpoint);
  }

  void pan(const glm::vec2 & dx)
  {
    this->position += this->orientation[0] * dx.x + this->orientation[1] * -dx.y;
  }

  void orbit(const glm::vec2 & dx)
  {
    const auto focalpoint = this->position - this->orientation[2] * this->focaldistance;

    auto rot = glm::mat4(this->orientation);
    rot = glm::rotate(rot, dx.y, glm::vec3(1, 0, 0));
    rot = glm::rotate(rot, dx.x, glm::vec3(0, 1, 0));
    const auto look = glm::mat3(rot) * glm::vec3(0, 0, this->focaldistance);

    this->position = focalpoint + look;
    this->lookAt(focalpoint);
  }

  void lookAt(const glm::vec3 & focalpoint)
  {
    this->orientation[2] = glm::normalize(this->position - focalpoint);
    this->orientation[0] = glm::normalize(glm::cross(this->orientation[1], this->orientation[2]));
    this->orientation[1] = glm::normalize(glm::cross(this->orientation[2], this->orientation[0]));
    this->focaldistance = glm::length(this->position - focalpoint);
  }

  void view(const box3 & box)
  {
    const auto focalpoint = box.center();
    this->focaldistance = glm::length(box.span());

    this->position = focalpoint + this->orientation[2] * this->focaldistance;
    this->lookAt(focalpoint);
  }

  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;
  float focaldistance;
  glm::vec3 position;
  glm::mat3 orientation;
  glm::mat4 ViewMatrix;
  glm::mat4 ProjMatrix;

  void updateMatrices()
  {
    this->ViewMatrix = glm::transpose(glm::mat4(this->orientation));
    this->ViewMatrix = glm::translate(this->ViewMatrix, -this->position);
    this->ProjMatrix = glm::perspective(this->fieldofview, this->aspectratio, this->nearplane, this->farplane);
  }
};

