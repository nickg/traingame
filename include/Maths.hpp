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

#if 0
template <typename T, int N>
union Packed;

template <>
union Packed<float, 4> {
   int __attribute__((mode(V4SF))) packed;
   float unpacked[4];
};

template <>
union Packed<float, 3> {
   int __attribute__((mode(V3SF))) packed;
   float unpacked[3];
};
#endif

// A generic 3D vector
template <typename T>
struct Vector {
   inline Vector(T x, T y, T z) : x(x), y(y), z(z) {}
   inline Vector() : x(0), y(0), z(0) {}

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
      return Vector<T>(x*t, y*t, z*t);
   }

   // Divide by a scalar
   inline Vector<T> operator/(T t) const
   {
      return Vector<T>(x/t, y/t, z/t);
   }

   // Scalar product
   inline T dot(const Vector<T>&v) const
   {
      return x*v.x + y*v.y + z*v.z;
   }

   // Magnitude
   inline T length() const
   {
      return sqrt(x*x + y*y + z*z);
   }

   inline Vector<T>& normalise()
   {
      T m = length();
      x /= m;
      y /= m;
      z /= m;
      return *this;
   }

   inline Vector<T> operator+(const Vector<T>& v) const
   {
      return Vector<T>(x+v.x, y+v.y, z+v.z);
   }

   inline Vector<T>& operator+=(const Vector<T>& v)
   {
      x += v.x;
      y += v.y;
      z += v.z;
      return *this;
   }
   
   inline Vector<T> operator-(const Vector<T>& v) const
   {
      return Vector<T>(x-v.x, y-v.y, z-v.z);
   }

   inline Vector<T> operator-() const
   {
      return Vector<T>(-x, -y, -z);
   }

   inline Vector<T>& operator-=(const Vector<T>& v)
   {
      x -= v.x;
      y -= v.y;
      z -= v.z;
      return *this;
   }
   
   inline bool operator==(const Vector<T>& v) const
   {
      return x == v.x && y == v.y && z == v.z;
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
   
   bool approx_equal(const Vector<T>& rhs, T delta) const
   {
      return (abs(rhs.x - x) < delta)
         && (abs(rhs.y - y) < delta)
         && (abs(rhs.z - z) < delta);
   }
   
   T x, y, z;
};

template <typename T>
std::ostream& operator<<(std::ostream& s, const Vector<T>& v)
{
   return s << "[" << v.x << " " << v.y
            << " " << v.z << "]";
}

template <typename T>
inline Vector<T> make_vector(T x, T y, T z)
{
   return Vector<T>(x, y, z);
}

typedef Vector<float> VectorF;

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
