#pragma once

#include <Innovator/Core/Misc/Defines.h>
#include <Innovator/Core/Math/Matrix.h>

using namespace Innovator::Core::Math;

class Camera {
public:
  NO_COPY_OR_ASSIGNMENT(Camera)
  ~Camera() = default;

  Camera()
  {
    this->farplane = 1000.0f;
    this->nearplane = 0.1f;
    this->aspectratio = 4.0f / 3;
    this->fieldofview = 0.7f;

    this->lookAt({ 5, 0, 5 }, { 0, 0, 0 }, { 0, 1, 0 });
  }

  void zoom(float dy)
  {
    this->p = this->p + mult({ this->x, this->y, this->z }, scale(this->z, dy));
  }

  void pan(const vec2f & dx)
  {
    this->p = this->p + mult({ this->x, this->y, this->z }, scale(this->x,  dx[0])) + 
                        mult({ this->x, this->y, this->z }, scale(this->y, -dx[1]));
  }

  void orbit(const vec2f & dx)
  {
    this->pan(dx);
    const auto r = transpose(mat3f{ this->x, this->y, this->z });
    this->lookAt(mult(r, this->p), { 0, 0, 0 }, this->y);
  }

  void lookAt(const vec3f & eye, const vec3f & target, const vec3f & up)
  {
    this->z = normalize(eye - target);
    this->x = normalize(cross(up, this->z));
    this->y = normalize(cross(this->z, this->x));
    this->p = mult({ this->x, this->y, this->z }, eye);
  }

  mat4f viewmatrix() const
  {
    return {
       this->x[0],  this->y[0],  this->z[0],  0,
       this->x[1],  this->y[1],  this->z[1],  0,
       this->x[2],  this->y[2],  this->z[2],  0,
      -this->p[0], -this->p[1], -this->p[2],  1,
    };
  }

  mat4f projmatrix() const
  {
    const auto m00 = 1.0f / (this->aspectratio * tan(this->fieldofview / 2));
    const auto m11 = 1.0f / tan(this->fieldofview / 2);
    const auto m23 = -1.0f;
    const auto m22 = -(this->farplane + this->nearplane) / (this->farplane - this->nearplane);
    const auto m32 = -(2 * this->farplane * this->nearplane) / (this->farplane - this->nearplane);

    return {
      m00, 0, 0, 0,
      0, m11, 0, 0,
      0, 0, m22, m23,
      0, 0, m32, 0,
    };
  }

  vec3f x, y, z, p;
  float farplane;
  float nearplane;
  float aspectratio;
  float fieldofview;
};

