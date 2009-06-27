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
   T length;  // A very rough approximation to the length

   BezierCurve(Vector<T> p1, Vector<T> p2, Vector<T> p3, Vector<T> p4)
   {
      p[0] = p1;
      p[1] = p2;
      p[2] = p3;
      p[3] = p4;
      
      // Approximate the length
      Vector<T> cur = operator()(0.0), prev;

      length = 0.0;
      
      const T step = 0.01;
      for (T t = 0.1; t <= 1.0; t += step) {
         prev = cur;
         cur = operator()(t);

         const Vector<T> diff = cur - prev;
         
         length += sqrt(diff.x*diff.x + diff.y*diff.y + diff.z*diff.z);
      }
   }

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

   // The derivative with respect to t at a point
   Vector<T> deriv(T t) const
   {
      return makeVector
         (// X
          p[0].x * -3 * (1 - t) * (1 - t)
          + p[1].x * (3 - 12*t + 9*t*t)
          + p[2].x * (6*t - 9*t*t)
          + p[3].x * 3 * t * t,

          // Y
          p[0].y * -3 * (1 - t) * (1 - t)
          + p[1].y * (3 - 12*t + 9*t*t)
          + p[2].y * (6*t - 9*t*t)
          + p[3].y * 3 * t * t,

          // Z
          p[0].z * -3 * (1 - t) * (1 - t)
          + p[1].z * (3 - 12*t + 9*t*t)
          + p[2].z * (6*t - 9*t*t)
          + p[3].z * 3 * t * t
          );
   }
};

// Generate Bezier curves
template <typename T>
BezierCurve<T> makeBezierCurve(Vector<T> p1, Vector<T> p2,
                               Vector<T> p3, Vector<T> p4)
{
   return BezierCurve<T>(p1, p2, p3, p4);
}

#endif
