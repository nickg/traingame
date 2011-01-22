//
//  Copyright (C) 2009-2011  Nick Gasson
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INC_MATHS_HPP
#define INC_MATHS_HPP

#include "Platform.hpp"

#include <cmath>
#include <ostream>
#include <cassert>

template <typename T, int N>
struct Packed;

template <>
struct Packed<float, 4> {
   typedef float __attribute__((vector_size (16))) Type;
};

template <>
struct Packed<float, 3> {
   typedef float __attribute__((vector_size (16))) Type;

   static inline Type pack(float a, float b, float c)
   {
      union {
         Type p;
         float f[4];
      } u;   
      u.f[0] = a;
      u.f[1] = b;
      u.f[2] = c;
      u.f[3] = 1.0f;
      return u.p;
   }
};

template <>
struct Packed<int, 3> {
   typedef int __attribute__((vector_size (16))) Type;
   
   static inline Type pack(int a, int b, int c)
   {
      union Union {
         Type p;
         int i[4];
      } u;
      u.i[0] = a;
      u.i[1] = b;
      u.i[2] = c;
      u.i[3] = 1;
      return u.p;
   }
};

template <typename T>
struct EqTolerance;

template <>
struct EqTolerance<float> {
   static const float Value = 0.01f;
};

template <>
struct EqTolerance<int> {
   static const int Value = 0;
};

template <typename T>
inline bool approx_equal(T a, T b)
{
   return abs(a - b) <= EqTolerance<T>::Value;
}

// A generic 3D vector
template <typename T>
struct Vector {
   explicit inline Vector(T x = 0, T y = 0, T z = 0)
   {
      packed = Packed<T, 3>::pack(x, y, z);
   }

   explicit inline Vector(const typename Packed<T, 3>::Type& v)
   {
      packed = v;
   }

   // Cross product
   inline Vector<T> operator*(const Vector<T>& v) const
   {
      return Vector<T>(
         y*v.z - z*v.y,
         z*v.x - x*v.z,
         x*v.y - y*v.x);
   }

   // Multiply by a scalar
   inline Vector<T> operator*(T t) const
   {
      return Vector<T>(packed * Packed<T, 3>::pack(t, t, t));
   }

   // Divide by a scalar
   inline Vector<T> operator/(T t) const
   {
      return Vector<T>(packed / Packed<T, 3>::pack(t, t, t));
   }

   // Scalar product
   inline T dot(const Vector<T>&v) const
   {
      const Vector<T> tmp(packed * v.packed);
      return tmp.x + tmp.y + tmp.z;
   }

   // Magnitude
   inline T length() const
   {
      const T prod = dot(*this);
      return sqrt(prod);
   }

   inline Vector<T>& normalise()
   {
      const T m = length();
      packed /= Packed<T, 3>::pack(m, m, m);
      return *this;
   }
   
   inline Vector<T> operator+(const Vector<T>& v) const
   {
      return Vector<T>(packed + v.packed);
   }
   
   inline Vector<T>& operator+=(const Vector<T>& v)
   {
      packed += v.packed;
      return *this;
   }
   
   inline Vector<T> operator-(const Vector<T>& v) const
   {
      return Vector<T>(packed - v.packed);
   }

   inline Vector<T> operator-() const
   {
      return Vector<T>(-packed);
   }

   inline Vector<T>& operator-=(const Vector<T>& v)
   {
      packed -= v.packed;
      return *this;
   }
   
   inline bool operator==(const Vector<T>& rhs) const
   {
      const typename Packed<T, 3>::Type diff = rhs.packed - packed;
      const T delta2 = EqTolerance<T>::Value * EqTolerance<T>::Value;

      const Vector<T> squared(diff * diff);
      
      return (squared.x <= delta2)
         && (squared.y <= delta2)
         && (squared.z <= delta2);
   }

   inline bool operator!=(const Vector<T>& v) const
   {
      return !(v == *this);
   }

   inline bool operator<(const Vector<T>& rhs) const
   {
      return x < rhs.x
         || (x == rhs.x
             && (y < rhs.y
                 || (y == rhs.y && z < rhs.z)));
   }

   union {
      typename Packed<T, 3>::Type packed;
      struct {
         T x, y, z;
      };
   };
};

typedef Vector<float> VectorF;
typedef Vector<double> VectorD;
typedef Vector<int> VectorI;

template <typename T>
std::ostream& operator<<(std::ostream& s, const Vector<T>& v)
{
   return s << "[" << v.x << " " << v.y << " " << v.z << "]";
}

template <typename T>
inline Vector<T> make_vector(T x, T y, T z)
{
   return Vector<T>(x, y, z);
}

// Find a surface normal
template <typename T>
Vector<T> surface_normal(const Vector<T>& a, const Vector<T>& b,
   const Vector<T>& c)
{
   const Vector<T> v1 = b - a;
   const Vector<T> v2 = c - a;
   Vector<T> n = v1 * v2;
   n.normalise();
   return n;
}

// Useful debugging function
void draw_normal(const Vector<float>& a_position,
                 const Vector<float>& a_normal);

// A 2D point in space
template <typename T>
struct Point {
   Point(T _x, T _y) : x(_x), y(_y) {}
   Point() : x(0), y(0) {}

   Point left() const { return Point(x - 1, y); }
   Point right() const { return Point(x + 1, y); }
   Point up() const { return Point(x, y + 1); }
   Point down() const { return Point(x, y - 1); }

   bool operator==(const Point<T>& a_point) const
   {
      return a_point.x == x && a_point.y == y;
   }

   bool operator!=(const Point<T>& rhs) const
   {
      return rhs.x != x || rhs.y != y;
   }

   Point<T> operator+(const Point<T>& rhs) const
   {
      return Point(x + rhs.x, y + rhs.y);
   }

   Point<T> operator-(const Point<T>& rhs) const
   {
      return Point(x - rhs.x, y - rhs.y);
   }
   
   Point<T>& operator+=(const Point<T>& rhs)
   {
      x += rhs.x;
      y += rhs.y;
      return *this;
   }
   
   Point<T>& operator-=(const Point<T>& rhs)
   {
      x -= rhs.x;
      y -= rhs.y;
      return *this;
   }

   Point<T> operator-() const
   {
      return make_point(-x, -y);
   }

   bool operator<(const Point<T>& rhs) const
   {
      return x < rhs.x
         || (x == rhs.x && y < rhs.y);
   }
   
   T x, y;
};

template <typename V, typename U>
inline Point<V> point_cast(const Point<U>& p)
{
   return make_point(static_cast<V>(p.x), static_cast<V>(p.y));
}

template <typename T>
std::ostream& operator<<(std::ostream& a_stream, const Point<T>& a_point)
{
   return a_stream << "(" << a_point.x << ", " << a_point.y << ")";
}

template <typename T>
Point<T> make_point(T x, T y)
{
   return Point<T>(x, y);
}

// A frustum
struct Frustum {
   bool point_in_frustum(float x, float y, float z);
   bool sphere_in_frustum(float x, float y, float z, float radius);
   bool cube_in_frustum(float x, float y, float z, float size);	// size = 0.5*side_length
   bool cuboid_in_frustum(float x,	  float y,	   float z,
      float sizeX, float sizeY, float sizeZ);
   
   float planes[6][4];
};

Frustum get_view_frustum();

// A rough guess at the gradient at a point on a curve
float approx_gradient(function<float (float)> a_func, float x);

// Useful functions for converting to/from radians

template <typename T>
inline float deg_to_rad(T t)
{
   return float(t) * M_PI / 180.0f;
}

template <typename T>
inline T rad_to_deg(float r)
{
   return T(r * 180.0f / M_PI);
}

#endif
