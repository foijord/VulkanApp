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
    this->e = this->e + this->x * dx.v[0] + this->y * dx.v[1];
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
      this->x.v[0], this->y.v[0], this->z.v[0], 0,
      this->x.v[1], this->y.v[1], this->z.v[1], 0,
      this->x.v[2], this->y.v[2], this->z.v[2], 0,
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
      m00, 0, 0,  0,
      0, -f, 0,   0,
      0, 0, m22, -1,
      0, 0, m32,  0,
    };
  }

  vec3d x, y, z, e, t;
  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;
};

