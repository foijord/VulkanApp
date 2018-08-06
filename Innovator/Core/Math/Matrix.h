#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <array>

namespace Innovator::Core::Math {

  template <typename T, int N>
  using vec = std::array<T, N>;

  template <typename T, int N>
  using mat = vec<vec<T, N>, N>;

  template <typename T>
  using vec4 = vec<T, 4>;

  template <typename T>
  using vec3 = vec<T, 3>;

  template <typename T>
  using mat3 = mat<T, 3>;

  template <typename T>
  using mat4 = mat<T, 4>;

  using vec3f = vec3<float>;
  using vec4f = vec4<float>;
  using mat3f = mat3<float>;
  using mat4f = mat4<float>;
  
  template <typename T, int N>
  vec<T, N> operator+(vec<T, N> v0, const vec<T, N> & v1)
  {
    for (auto i = 0; i < N; i++) {
      v0[i] += v1[i];
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

  template <typename T>
  mat4<T> new_mat4()
  {
    return {
       1, 0, 0, 0,
       0, 1, 0, 0,
       0, 0, 1, 0,
       0, 0, 0, 1 
    };
  }

  template <typename T>
  mat4<T> translate(mat4<T> m, const vec4<T> & v)
  {
    m[3] = m[3] + v;
    return m;
  }

  template <typename T, int N>
  mat<T, N> scale(mat<T, N> m, const vec<T, N> & v)
  {
    m[0][0] *= v[0];
    m[1][1] *= v[1];
    m[2][2] *= v[2];

    return m;
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
  mat<T, N> mult(const mat<T, N> & m0, const mat<T, N> & m1)
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
}

template <typename T>
Innovator::Core::Math::mat4<T> convert(const glm::mat4 & m)
{
  Innovator::Core::Math::mat4<T> mat;
  memcpy(mat.data(), glm::value_ptr(m), sizeof(T) * 16);
  return mat;
}

