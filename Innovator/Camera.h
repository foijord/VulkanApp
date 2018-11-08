#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Math/Matrix.h>

using namespace Innovator::Core::Math;

class Camera {
public:
  NO_COPY_OR_ASSIGNMENT(Camera)
  ~Camera() = default;

  Camera(float farplane, float nearplane, float aspectratio, float fieldofview)
    : x{ 1, 0, 0 },
      y{ 0, 1, 0 },
      z{ 0, 0, 1 },
      e{ 0, 0, 0 },
      t{ 0, 0, 0 },
      farplane(farplane),
      nearplane(nearplane),
      aspectratio(aspectratio),
      fieldofview(fieldofview)
  {}

  void zoom(double dy)
  {
    this->e = this->e + this->z * dy;
  }

  void pan(const vec2d & dx)
  {
    this->e = this->e + this->x * dx[0] + this->y * dx[1];
  }

  void orbit(const vec2d & dx)
  {
    this->pan(dx);
    this->lookAt(this->e, this->t, this->y);
  }

  void lookAt(const vec3d & eye, const vec3d & target, const vec3d & up)
  {
    this->y = up;
    this->e = eye;
    this->t = target;

    this->z = normalize(this->e - this->t);
    this->x = normalize(this->y ^ this->z);
    this->y = normalize(this->z ^ this->x);
  }

  mat4d viewmatrix() const
  {
    return {
      this->x[0], this->y[0], this->z[0], 0,
      this->x[1], this->y[1], this->z[1], 0,
      this->x[2], this->y[2], this->z[2], 0,
       -(x * e),   -(y * e),   -(z * e),  1,
    };
  }

  mat4d projmatrix() const
  {
    const auto f = 1.0 / tan(this->fieldofview / 2);
    const auto m00 = f / this->aspectratio;
    const auto m22 = this->farplane / (this->nearplane - this->farplane);
    const auto m32 = (this->nearplane * this->farplane) / (this->nearplane - this->farplane);

    return {
      vec4d{ m00, 0, 0,  0 },
      vec4d{ 0, -f, 0,   0 },
      vec4d{ 0, 0, m22, -1 },
      vec4d{ 0, 0, m32,  0 },
    };
  }

  vec3d x, y, z, e, t;
  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;
};

