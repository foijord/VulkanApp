#pragma once

#include <array>
#include <cmath>

namespace Innovator {
  namespace Core {
    namespace Math {

      template <typename T, int N> class vec;
      template <typename T, int N> class mat;

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
      class vec {
      public:
        const vec<T, N> & operator += (const vec<T, N> & v) {
          for (auto i = 0; i < N; i++) {
            this->v[i] += v.v[i];
          }
          return *this;
        }

        const vec<T, N> & operator -= (const vec<T, N> & v) {
          for (auto i = 0; i < N; i++) {
            this->v[i] -= v.v[i];
          }
          return *this;
        }

        const vec<T, N> & operator *= (T s) {
          for (size_t i = 0; i < N; i++) {
            this->v[i] *= s;
          }
          return *this;
        }

        const vec<T, N> & operator /= (T s) {
          for (size_t i = 0; i < N; i++) {
            this->v[i] /= s;
          }
          return *this;
        }

        T length() {
          return sqrt((*this) * (*this));
        }

        const vec<T, N> & normalize()
        {
          (*this) /= this->length();
          return *this;
        }

        const vec<T, N> & negate()
        {
          (*this) *= T(-1);
          return *this;
        }

        T v[N];
      };

      template <typename T, int N>
      vec<T, N> operator + (vec<T, N> v0, const vec<T, N> & v1)
      {
        return v0 += v1;
      }

      template <typename T, int N>
      vec<T, N> operator - (vec<T, N> v0, const vec<T, N> & v1)
      {
        return v0 -= v1;
      }

      template <typename T, int N>
      T operator * (const vec<T, N> & v0, const vec<T, N> & v1)
      {
        T d = 0;
        for (size_t i = 0; i < N; i++) {
          d += v0.v[i] * v1.v[i];
        }
        return d;
      }

      template <typename T, int N>
      vec<T, N> operator * (vec<T, N> v, T s)
      {
        return v *= s;
      }

      template <typename T, int N>
      T length(vec<T, N> v)
      {
        return v.length();
      }

      template <typename T, int N>
      vec<T, N> normalize(vec<T, N> v)
      {
        return v.normalize();
      }

      template <typename T, int N>
      vec<T, N> negate(vec<T, N> v)
      {
        return v.negate();
      }

      template <typename T>
      vec3<T> operator^(const vec3<T> & v0, const vec3<T> & v1)
      {
        return {
          v0.v[1] * v1.v[2] - v0.v[2] * v1.v[1],
          v0.v[2] * v1.v[0] - v0.v[0] * v1.v[2],
          v0.v[0] * v1.v[1] - v0.v[1] * v1.v[0],
        };
      }

      template <typename T, int N>
      class mat {
      public:
        const mat<T, N> & transpose()
        {
          mat<T, N> t;
          for (size_t i = 0; i < N; i++) {
            for (size_t j = 0; j < N; j++) {
              t.m[i].v[j] = this->m[j].v[i];
            }
          }
          *this = t;
          return *this;
        }

        vec<T,N> m[N];
      };

      template <typename T, int N>
      mat<T, N> transpose(mat<T, N> m)
      {
        return m.transpose();
      }

      template <typename T, int N>
      vec<T, N> operator * (const mat<T, N> & m, const vec<T, N> & v)
      {
        vec<T, N> result;
        for (size_t i = 0; i < N; i++) {
          result.m[i] = v * m.m[i];
        }
        return result;
      }

      template <typename T, int N>
      mat<T, N> operator * (const mat<T, N> & m0, const mat<T, N> & m1)
      {
        mat<T, N> t = transpose(m1);
        mat<T, N> m;
        for (size_t i = 0; i < N; i++) {
          for (size_t j = 0; j < N; j++) {
            m.m[i].v[j] = m0.m[i] * t.m[j];
          }
        }
        return m;
      }

      template <typename T0, typename T1, int N>
      vec<T0, N> cast(const vec<T1, N> & v)
      {
        vec<T0, N> vec;
        for (size_t i = 0; i < N; i++) {
          vec.v[i] = static_cast<T0>(v.v[i]);
        }
        return vec;
      }

      template <typename T0, typename T1, int N>
      mat<T0, N> cast(const mat<T1, N> & m)
      {
        mat<T0, N> mat;
        for (size_t i = 0; i < N; i++) {
          mat.m[i] = cast<T0, T1, N>(m.m[i]);
        }
        return mat;
      }
    }
  }
}
