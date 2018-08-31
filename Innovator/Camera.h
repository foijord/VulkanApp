#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Math/Matrix.h>

using namespace Innovator::Core::Math;

class Camera {
public:
  NO_COPY_OR_ASSIGNMENT(Camera)
  ~Camera() = default;

  explicit Camera() :
    farplane(1000.0f),
    nearplane(0.1f),
    aspectratio(4.0f / 3.0f),
    fieldofview(0.7f)
  {
    this->lookAt({ 0, 0, 0 }, { 0, 0, -1 }, { 0, 1, 0 });
  }

  void zoom(float dy)
  {
    this->position = this->position + scale(this->z, dy);
  }

  void pan(const vec2f & dx)
  {
    this->position = this->position + scale(this->x, dx[0]) + scale(this->y, -dx[1]);
  }

  void orbit(const vec2f & dx)
  {
    this->pan(dx);

    this->z = normalize(this->position - this->focalpoint);
    this->x = normalize<float>(cross<float>(this->y, this->z));
    this->y = normalize<float>(cross<float>(this->z, this->x));
  }

  void lookAt(const vec3f & position, const vec3f & focalpoint, const vec3f & up)
  {
    this->position = position;
    this->focalpoint = focalpoint;
    this->z = normalize(this->position - this->focalpoint);
    this->x = normalize<float>(cross<float>(up, this->z));
    this->y = normalize<float>(cross<float>(this->z, this->x));
  }

  mat4f viewmatrix() const
  {
    return transpose(mat4f{
      x[0], x[1], x[2], -position[0],
      y[0], y[1], y[2], -position[1],
      z[0], z[1], z[2], -position[2],
         0,    0,    0,            1,
    });
  }

  mat4f projmatrix() const
  {
    return perspective(this->fieldofview, this->aspectratio, this->nearplane, this->farplane);
  }

  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;

  vec3f position;
  vec3f focalpoint;
  vec3f x, y, z;
};

