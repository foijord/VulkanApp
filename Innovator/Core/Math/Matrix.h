#pragma once

#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>

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

  template <typename T>
  mat4<T> rotate(const mat4<T> & m, T a, const vec3<T> & v)
  {
    T const c = cos(a);
    T const s = sin(a);

    vec3<T> axis = normalize(v);
    vec3<T> temp = scale<T, 3>(axis, (T(1) - c));

    mat4<T> rot;
    rot[0][0] = c + temp[0] * axis[0];
    rot[0][1] = temp[0] * axis[1] + s * axis[2];
    rot[0][2] = temp[0] * axis[2] - s * axis[1];

    rot[1][0] = temp[1] * axis[0] - s * axis[2];
    rot[1][1] = c + temp[1] * axis[1];
    rot[1][2] = temp[1] * axis[2] + s * axis[0];

    rot[2][0] = temp[2] * axis[0] + s * axis[1];
    rot[2][1] = temp[2] * axis[1] - s * axis[0];
    rot[2][2] = c + temp[2] * axis[2];

    mat4<T> result;
    result[0] = m[0] * rot[0][0] + m[1] * rot[0][1] + m[2] * rot[0][2];
    result[1] = m[0] * rot[1][0] + m[1] * rot[1][1] + m[2] * rot[1][2];
    result[2] = m[0] * rot[2][0] + m[1] * rot[2][1] + m[2] * rot[2][2];
    result[3] = m[3];

    return result;
  }

  template <typename T>
  mat4<T> frustum(T left, T right, T bottom, T top, T nearVal, T farVal)
  {
    mat4f Result;
    Result[0][0] = (static_cast<T>(2) * nearVal) / (right - left);
    Result[1][1] = (static_cast<T>(2) * nearVal) / (top - bottom);
    Result[2][0] = (right + left) / (right - left);
    Result[2][1] = (top + bottom) / (top - bottom);
    Result[2][3] = static_cast<T>(-1);
    Result[2][2] = -(farVal + nearVal) / (farVal - nearVal);
    Result[3][2] = -(static_cast<T>(2) * farVal * nearVal) / (farVal - nearVal);

    return Result;
  }

  template <typename T>
  mat4<T> perspective(T fovy, T aspect, T zNear, T zFar)
  {
    assert(abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat4f Result = {
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
      0, 0, 0, 0,
    };

    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][3] = -static_cast<T>(1);
    Result[2][2] = -(zFar + zNear) / (zFar - zNear);
    Result[3][2] = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);

    return Result;
  }
}

template <typename T>
Innovator::Core::Math::vec4<T> vec3_to_vec4(const Innovator::Core::Math::vec3<T> & v)
{
  return { v[0], v[1], v[2], 0 };
}

template <typename T>
Innovator::Core::Math::mat3<T> mat4_to_mat3(const Innovator::Core::Math::mat4<T> & m)
{
  return {
    m[0][0], m[1][0], m[2][0],
    m[0][1], m[1][1], m[2][1],
    m[0][2], m[1][2], m[2][2]
  };
}

template <typename T>
Innovator::Core::Math::mat4<T> mat3_to_mat4(const Innovator::Core::Math::mat3<T> & m)
{
  return {
    m[0][0], m[1][0], m[2][0], 0,
    m[0][1], m[1][1], m[2][1], 0,
    m[0][2], m[1][2], m[2][2], 0,
          0,       0,       0, 1,
  };
}

template <typename T>
Innovator::Core::Math::mat4<T> convert(const glm::mat4 & m)
{
  Innovator::Core::Math::mat4<T> mat;
  memcpy(mat.data(), glm::value_ptr(m), sizeof(T) * 16);
  return mat;
}

template <typename T>
Innovator::Core::Math::mat3<T> convert(const glm::mat3 & m)
{
  Innovator::Core::Math::mat3<T> mat;
  memcpy(mat.data(), glm::value_ptr(m), sizeof(T) * 9);
  return mat;
}

template <typename T>
Innovator::Core::Math::vec4<T> convert(const glm::vec4 & m)
{
  Innovator::Core::Math::vec4<T> vec;
  memcpy(vec.data(), glm::value_ptr(m), sizeof(T) * 4);
  return vec;
}

template <typename T>
Innovator::Core::Math::vec3<T> convert(const glm::vec3 & m)
{
  Innovator::Core::Math::vec3<T> vec;
  memcpy(vec.data(), glm::value_ptr(m), sizeof(T) * 3);
  return vec;
}

template <typename T>
glm::vec3 convert(const Innovator::Core::Math::vec3<T> & v)
{
  glm::vec3 vec;
  memcpy(glm::value_ptr(vec), v.data(), sizeof(T) * 3);
  return vec;
}

template <typename T>
glm::mat3 convert(const Innovator::Core::Math::mat3<T> & m)
{
  glm::mat3 mat;
  memcpy(glm::value_ptr(mat), m.data(), sizeof(T) * 9);
  return mat;
}

template <typename T>
glm::mat4 convert(const Innovator::Core::Math::mat4<T> & m)
{
  glm::mat4 mat;
  memcpy(glm::value_ptr(mat), m.data(), sizeof(T) * 16);
  return mat;
}
