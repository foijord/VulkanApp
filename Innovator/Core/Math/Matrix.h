#pragma once

#include <array>
#include <cmath>

namespace Innovator {
  namespace Core {
    namespace Math {

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

      using vec2d = vec2<double>;
      using vec3d = vec3<double>;
      using vec4d = vec4<double>;
      using mat3d = mat3<double>;
      using mat4d = mat4<double>;

      template <typename T, int N>
      vec<T, N> operator + (vec<T, N> v0, const vec<T, N> & v1)
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
        return v * (T(1) / length(v));
      }

      template <typename T, int N>
      vec<T, N> negate(const vec<T, N> & v)
      {
        return scale(v, T(-1));
      }

      template <typename T>
      vec3<T> operator^(const vec3<T> & v0, const vec3<T> & v1)
      {
        return {
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
      vec<T, N> operator * (const mat<T, N> & m, const vec<T, N> & v)
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

      template <typename T0, typename T1, int N>
      vec<T0, N> cast(const vec<T1, N> & v)
      {
        vec<T0, N> vec;
        for (auto i = 0; i < N; i++) {
          vec[i] = static_cast<T0>(v[i]);
        }
        return vec;
      }

      template <typename T0, typename T1, int N>
      mat<T0, N> cast(const mat<T1, N> & m)
      {
        mat<T0, N> mat;
        for (auto i = 0; i < N; i++) {
          mat[i] = cast<T0, T1, N>(m[i]);
        }
        return mat;
      }
    }
  }
}
