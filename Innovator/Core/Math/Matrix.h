#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>

namespace Innovator::Core::Math {

  typedef std::array<float, 4> vec4;
  typedef std::array<vec4, 4> mat4;

  inline vec4 add(const vec4 & v0, const vec4 & v1)
  {
    vec4 v;
    for (auto i = 0; i < 4; i++) {
      v[i] = v0[i] + v1[i];
    }
    return v;
  }

  inline float dot(const vec4 & v0, const vec4 & v1)
  {
    float d = 0;
    for (auto i = 0; i < 4; i++) {
      d += v0[i] * v1[i];
    }
    return d;
  }

  inline mat4 identity()
  {
    return {
       1, 0, 0, 0,
       0, 1, 0, 0,
       0, 0, 1, 0,
       0, 0, 0, 1 
    };
  }

  inline mat4 translate(mat4 m, const vec4 & v)
  {
    m[3] = add(m[3], v);
    return m;
  }

  inline mat4 scale(mat4 m, const vec4 & v)
  {
    m[0][0] *= v[0];
    m[1][1] *= v[1];
    m[2][2] *= v[2];

    return m;
  }

  inline mat4 transpose(const mat4 & m)
  {
    mat4 t;
    for (auto i = 0; i < 4; i++) {
      for (auto j = 0; j < 4; j++) {
        t[i][j] = m[j][i];
      }
    }
    return t;
  }

  inline mat4 mult(const mat4 & m0, const mat4 & m1)
  {
    mat4 t = transpose(m1);
    mat4 m;
    for (auto i = 0; i < 4; i++) {
      for (auto j = 0; j < 4; j++) {
        m[i][j] = dot(m0[i], t[j]);
      }
    }
    return m;
  }
}

inline Innovator::Core::Math::mat4 convert(const glm::mat4 & m)
{
  Innovator::Core::Math::mat4 mat;
  memcpy(mat.data(), glm::value_ptr(m), sizeof(float) * 16);
  return mat;
}

