#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Math/Box.h>
#include <Innovator/Core/Math/Matrix.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace Innovator::Core::Math;

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
    position({ 0, 0, 1 }),
    orientation(glm::mat3(1.0))
  {}

  void zoom(float dy)
  {
    const auto direction = convert<float>(this->orientation[2]);

    const auto focalpoint = this->position - scale(direction, this->focaldistance);
    this->position = this->position + scale(direction, dy);
    this->focaldistance = length(this->position - focalpoint);
  }

  void pan(const vec2f & dx)
  {
    const auto x = convert<float>(this->orientation[0]);
    const auto y = convert<float>(this->orientation[1]);

    this->position = this->position + scale(x, dx[0]) + scale(y, -dx[1]);
  }

  void orbit(const vec2f & dx)
  {
    const auto direction = convert<float>(this->orientation[2]);

    const auto focalpoint = this->position - scale(direction, this->focaldistance);

    mat4f rot = convert<float>(glm::mat4(this->orientation));
    rot = rotate(rot, dx[1], vec3f{ 1, 0, 0 });
    rot = rotate(rot, dx[0], vec3f{ 0, 1, 0 });
    vec3f look = mult(mat4_to_mat3<float>(rot), vec3f { 0, 0, this->focaldistance });

    this->position = focalpoint + look;
    this->lookAt(focalpoint);
  }

  void lookAt(const vec3f & focalpoint)
  {
    this->orientation[2] = convert(normalize(this->position - focalpoint));
    this->orientation[0] = glm::normalize(glm::cross(this->orientation[1], this->orientation[2]));
    this->orientation[1] = glm::normalize(glm::cross(this->orientation[2], this->orientation[0]));
    this->focaldistance = glm::length(convert(this->position) - convert(focalpoint));
  }

  void view(const box3 & box)
  {
    const auto focalpoint = box.center();
    this->focaldistance = glm::length(box.span());

    this->position = convert<float>(focalpoint + this->orientation[2] * this->focaldistance);
    this->lookAt(convert<float>(focalpoint));
  }

  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;
  float focaldistance;
  vec3f position;
  glm::mat3 orientation;
  mat4f ViewMatrix;
  mat4f ProjMatrix;

  void updateMatrices()
  {
    this->ViewMatrix = convert<float>(glm::transpose(glm::mat4(this->orientation)));
    this->ViewMatrix = convert<float>(glm::translate(convert(this->ViewMatrix), -convert(this->position)));
    this->ProjMatrix = convert<float>(glm::perspective(this->fieldofview, this->aspectratio, this->nearplane, this->farplane));
  }
};

