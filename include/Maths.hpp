//
//  Copyright (C) 2009  Nick Gasson
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

#include <cmath>
#include <ostream>

// A generic 3D vector
template <typename T>
struct Vector {
   Vector(T x, T y, T z) : x(x), y(y), z(z) {}
   Vector() : x(0), y(0), z(0) {}

   // Cross product
   Vector<T> operator*(const Vector<T>& v) const
   {
      return Vector<T>(y*v.z - z*v.y,
                       z*v.x - x*v.z,
                       x*v.y - y*v.x);
   }

   // Multiply by a scalar
   Vector<T> operator*(T t) const
   {
      return Vector<T>(x*t, y*t, z*t);
   }

   // Scalar product
   T dot(const Vector<T>&v) const
   {
      return x*v.x + y*v.y + z*v.z;
   }

   // Magnitude
   T magnitude() const
   {
      return static_cast<T>(sqrt(x*x + y*y + z*z));
   }

   Vector<T>& normalise()
   {
      T m = magnitude();
      x /= m;
      y /= m;
      z /= m;
      return *this;
   }

   Vector<T> operator+(const Vector<T>& v) const
   {
      return Vector<T>(x+v.x, y+v.y, z+v.z);
   }

   Vector<T> operator+=(const Vector<T>& v)
   {
      x += v.x;
      y += v.y;
      z += v.z;
      return *this;
   }
   
   Vector<T> operator-(const Vector<T>& v) const
   {
      return Vector<T>(x-v.x, y-v.y, z-v.z);
   }

   Vector<T> operator-() const
   {
      return Vector<T>(-x, -y, -z);
   }

   Vector<T> operator-=(const Vector<T>& v)
   {
      x -= v.x;
      y -= v.y;
      z -= v.z;
      return *this;
   }
   
   bool operator==(const Vector<T>& v) const
   {
      return x == v.x && y == v.y && z == v.z;
   }

   bool operator!=(const Vector<T>& v) const
   {
      return !(v == *this);
   }
   
   T x, y, z;
};

template <typename T>
std::ostream& operator<<(std::ostream& aStream, const Vector<T>& aVector)
{
   return aStream << "[" << aVector.x << " " << aVector.y
                  << " " << aVector.z << "]";
}

template <typename T>
Vector<T> makeVector(T x, T y, T z)
{
   return Vector<T>(x, y, z);
}

// Find a surface normal
template <typename T>
Vector<T> surfaceNormal(const Vector<T>& a, const Vector<T>& b,
                        const Vector<T>& c)
{
   Vector<T> v1 = b - a;
   Vector<T> v2 = c - a;
   Vector<T> n = v1 * v2;
   n.normalise();
   return n;
}

// Useful debugging function
void drawNormal(const Vector<double>& aPosition,
                const Vector<double>& aNormal);

// A 2D point in space
template <typename T>
struct Point {
   Point(T _x, T _y) : x(_x), y(_y) {}
   Point() : x(0), y(0) {}

   Point left() const { return Point(x - 1, y); }
   Point right() const { return Point(x + 1, y); }
   Point up() const { return Point(x, y + 1); }
   Point down() const { return Point(x, y - 1); }

   bool operator==(const Point<T>& aPoint) const
   {
      return aPoint.x == x && aPoint.y == y;
   }
   
   T x, y;
};

template <typename T>
std::ostream& operator<<(std::ostream& aStream, const Point<T>& aPoint)
{
   return aStream << "(" << aPoint.x << ", " << aPoint.y << ")";
}

template <typename T>
Point<T> makePoint(T x, T y)
{
   return Point<T>(x, y);
}

// A frustum
struct Frustum {
   bool pointInFrustum(float x, float y, float z);
   bool sphereInFrustum(float x, float y, float z, float radius);
   bool cubeInFrustum(float x, float y, float z, float size);	// size = 0.5*side_length
   bool cuboidInFrustum(float x,	  float y,	   float z,
                        float sizeX, float sizeY, float sizeZ);
   
   float planes[6][4];
};

Frustum getViewFrustum();

// Useful functions for converting to/from radians

template <typename T>
inline double degToRad(T t)
{
   return static_cast<double>(t) * M_PI / 180.0;
}

template <typename T>
inline T radToDeg(double r)
{
   return static_cast<T>(r * 180.0 / M_PI);
}

#endif
