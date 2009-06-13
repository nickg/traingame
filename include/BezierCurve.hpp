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

#ifndef INC_BEZIER_CURVE_HPP
#define INC_BEZIER_CURVE_HPP

#include "Platform.hpp"
#include "Maths.hpp"

// Three-dimensional Bezier curve
template <typename T>
struct BezierCurve {
   Vector<T> p[4];

   Vector<T> operator()(T t) const
   {
      return makeVector
         (// X
          p[0].x * (1 - t) * (1 - t) * (1 - t)
          + p[1].x * 3 * t * (1 - t) * (1 - t)
          + p[2].x * 3 * t * t * (1 - t)
          + p[3].x * t * t * t,

          // Y
          p[0].y * (1 - t) * (1 - t) * (1 - t)
          + p[1].y * 3 * t * (1 - t) * (1 - t)
          + p[2].y * 3 * t * t * (1 - t)
          + p[3].y * t * t * t,

          // Z
          p[0].z * (1 - t) * (1 - t) * (1 - t)
          + p[1].z * 3 * t * (1 - t) * (1 - t)
          + p[2].z * 3 * t * t * (1 - t)
          + p[3].z * t * t * t
          );
   }
};

// Generate Bezier curves
template <typename T>
BezierCurve<T> makeBezierCurve(Vector<T> p1, Vector<T> p2,
                               Vector<T> p3, Vector<T> p4)
{
   BezierCurve<T> b = { { p1, p2, p3, p4 } };
   return b;
}

#endif

