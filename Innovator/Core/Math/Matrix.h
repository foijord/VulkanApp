#pragma once

#include <array>
#include <cmath>

namespace Innovator::Core::Math {

  template <typename T, int N>
  using vec = std::array<T, N>;

  template <typename T, int N>
  using mat = vec<vec<T, N>, N>;

  template <typename T>
  using vec2 = vec<T, 2>;

  template <typename T>
  using vec3 = vec<T, 3>;

  template <typename T>
  using vec4 = vec<T, 4>;

  template <typename T>
  using mat3 = mat<T, 3>;

  template <typename T>
  using mat4 = mat<T, 4>;

  using vec2f = vec2<float>;
  using vec3f = vec3<float>;
  using vec4f = vec4<float>;
  using mat3f = mat3<float>;
  using mat4f = mat4<float>;

  template <typename T, int N>
  vec<T, N> operator + (vec<T, N> v0, const vec<T, N> & v1)
  {
    for (auto i = 0; i < N; i++) {
      v0[i] += v1[i];
    }
    return v0;
  }

  template <typename T, int N>
  vec<T, N> operator + (vec<T, N> v0, const vec<T, N + 1> & v1)
  {
    for (auto i = 0; i < N; i++) {
      v0[i] += v1[i];
    }
    return v0;
  }

  template <typename T, int N>
  vec<T, N> operator - (vec<T, N> v0, const vec<T, N> & v1)
  {
    for (auto i = 0; i < N; i++) {
      v0[i] -= v1[i];
    }
    return v0;
  }

  template <typename T, int N>
  T operator * (const vec<T, N> & v0, const vec<T, N> & v1)
  {
    T d = 0;
    for (auto i = 0; i < N; i++) {
      d += v0[i] * v1[i];
    }
    return d;
  }

  template <typename T, int N>
  vec<T, N> scale(vec<T, N> v, T s)
  {
    for (auto i = 0; i < N; i++) {
      v[i] *= s;
    }
    return v;
  }

  template <typename T, int N>
  vec<T, N> operator * (vec<T, N> v, T s)
  {
    for (auto i = 0; i < N; i++) {
      v[i] *= s;
    }
    return v;
  }

  template <typename T, int N>
  T length(const vec<T, N> & v)
  {
    return sqrt(v * v);
  }

  template <typename T, int N>
  vec<T, N> normalize(const vec<T, N> & v)
  {
    return scale(v, T(1) / length(v));
  }

  template <typename T>
  mat4<T> translate(const mat4<T> & m, const vec3<T> & v)
  {
    mat4f Result = m;
    Result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
    return Result;
  }

  template <typename T, int N>
  mat<T, N> scale(mat<T, N> m, const vec<T, N> & v)
  {
    m[0][0] *= v[0];
    m[1][1] *= v[1];
    m[2][2] *= v[2];

    return m;
  }

  template <typename T>
  vec3f cross(const vec3f & v0, const vec3f & v1)
  {
    return vec3<T>{
      v0[1] * v1[2] - v0[2] * v1[1],
      v0[2] * v1[0] - v0[0] * v1[2],
      v0[0] * v1[1] - v0[1] * v1[0],
    };
  }
  template <typename T, int N>
  mat<T, N> transpose(const mat<T, N> & m)
  {
    mat<T, N> t;
    for (auto i = 0; i < N; i++) {
      for (auto j = 0; j < N; j++) {
        t[i][j] = m[j][i];
      }
    }
    return t;
  }

  template <typename T, int N>
  vec<T, N> mult (mat<T, N> m, const vec<T, N> & v)
  {
    vec<T, N> result;
    for (auto i = 0; i < N; i++) {
      result[i] = v * m[i];
    }
    return result;
  }

  template <typename T, int N>
  mat<T, N> operator * (const mat<T, N> & m0, const mat<T, N> & m1)
  {
    mat<T, N> t = transpose(m1);
    mat<T, N> m;
    for (auto i = 0; i < N; i++) {
      for (auto j = 0; j < N; j++) {
        m[i][j] = m0[i] * t[j];
      }
    }
    return m;
  }

  inline mat4f perspective(float fovy, float aspect, float znear, float zfar)
  {
    const auto m00 = 1.0f / (aspect * tan(fovy / 2));
    const auto m11 = 1.0f / tan(fovy / 2);
    const auto m23 = -1.0f;
    const auto m22 = -(zfar + znear) / (zfar - znear);
    const auto m32 = -(2 * zfar * znear) / (zfar - znear);

    return {
      m00, 0, 0, 0,
      0, m11, 0, 0,
      0, 0, m22, m23,
      0, 0, m32, 0,
    };
  }
}
