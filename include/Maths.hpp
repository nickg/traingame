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

// A generic 3D vector
template <typename T>
struct Vector {
   Vector(T x, T y, T z) : x(x), y(y), z(z) {}
   Vector() : x(0), y(0), z(0) {}
   
   T x, y, z;
};

template <typename T>
Vector<T> makeVector(T x, T y, T z)
{
   return Vector<T>(x, y, z);
}

// A 2D point in space
template <typename T>
struct Point {
   T x, y;
};

template <typename T>
Point<T> makePoint(T x, T y)
{
   return Point<T>(x, y);
}

// A frustum
template <typename T>
struct Frustum {
   T planes[6][4];
};

#endif
