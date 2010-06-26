//
//  Copyright (C) 2010  Nick Gasson
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

#ifndef INC_MATRIX_HPP
#define INC_MATRIX_HPP

#include "Platform.hpp"
#include "Maths.hpp"

#include <ostream>
#include <cmath>

template <typename T, int N>
struct Matrix {
   Matrix(const T data[N][N])
   {
      for (int i = 0; i < N; i++) {
         for (int j = 0; j < N; j++)
            entries[i][j] = data[i][j];
      }
   }

   Matrix()
   {
      for (int i = 0; i < N; i++) {
         for (int j = 0; j < N; j++)
            entries[i][j] = 0;
      }
   }

   Matrix(const Matrix& rhs)
   {
      for (int i = 0; i < N; i++) {
         for (int j = 0; j < N; j++)
            entries[i][j] = rhs.entries[i][j];
      }
   }
    
   // TODO: these factories should work for N != 4
    
   static Matrix<T, 4> identity()
   {
      const T data[4][4] = {
         { 1, 0, 0, 0 },
         { 0, 1, 0, 0 },
         { 0, 0, 1, 0 },
         { 0, 0, 0, 1 }
      };
      return Matrix<T, 4>(data);
   }

   static Matrix<T, 4> translation(T x, T y, T z)
   {
      const T data[4][4] = {
         { 1, 0, 0, x },
         { 0, 1, 0, y },
         { 0, 0, 1, z },
         { 0, 0, 0, 1 }
      };
      return Matrix<T, 4>(data);
   }

   static Matrix<T, 4> rotation(float a, int x, int y, int z)
   {
      a *= M_PI / 180;

      assert(x + y + z == 1);
	
      if (x == 1) { 
         const T data[4][4] = {
            { 1, 0,       0,      0 },
            { 0, cos(a), -sin(a), 0 },
            { 0, sin(a),  cos(a), 0 },
            { 0, 0,       0,      1 }
         };
         return Matrix<T, 4>(data);
      }
      else if (y == 1) {
         const T data[4][4] = {
            {  cos(a), 0,  sin(a), 0 },
            {  0,      1,  0,      0 },
            { -sin(a), 0,  cos(a), 0 },
            {  0,      0,  0,      1 }
         };
         return Matrix<T, 4>(data);
      }
      else if (z == 1) {
         const T data[4][4] = {
            { cos(a), -sin(a), 0, 0 },
            { sin(a),  cos(a), 0, 0 },
            { 0,       0,      1, 0 },
            { 0,       0,      0, 1 }
         };
         return Matrix<T, 4>(data);
      }
      else
         assert(false);
   }

   Vector<T> transform(const Vector<T>& v) const
   {
      assert(N == 4);

      T v_cols[4] = { v.x, v.y, v.z, 1 };
      T cols[4];
      for (int i = 0; i < N; i++) {
         cols[i] = 0;
         for (int j = 0; j < N; j++)
            cols[i] += entries[i][j] * v_cols[j];
      }

      return make_vector(cols[0], cols[1], cols[2]);
   }

   Matrix<T, N>& operator*=(const Matrix<T, N>& rhs)
   {
      return *this = *this * rhs;
   }
    
   Matrix<T, N> operator*(const Matrix<T, N>& rhs) const
   {
      // Lame matrix multiplication algorithm
      T c[N][N];

      for (int i = 0; i < N; i++) {
         for (int j = 0; j < N; j++) {
            c[i][j] = 0;
            for (int k = 0; k < N; k++)
               c[i][j] += entries[i][k] * rhs.entries[k][j];
         }
      }

      return Matrix<T, N>(c);
   }

   T entries[N][N];  // Square matrices only
};

template <typename T>
Vector<T> rotate(Vector<T> v, T a, T x, T y, T z)
{
   Matrix<T, 4> r = Matrix<T, 4>::rotation(a, x, y, z);
   return r.transform(v);
}

template <typename T>
inline Vector<T> rotateX(Vector<T> v, T a)
{
   return rotate(v, a, 1.0f, 0.0f, 0.0f);
}

template <typename T>
inline Vector<T> rotateY(Vector<T> v, T a)
{
   return rotate(v, a, 0.0f, 1.0f, 0.0f);
}

template <typename T>
inline Vector<T> rotateZ(Vector<T> v, T a)
{
   return rotate(v, a, 0.0f, 0.0f, 1.0f);
}

template <typename T, int N>
ostream& operator<<(ostream& os, const Matrix<T, N>& m)
{
   for (int i = 0; i < N; i++) {
      os << endl;
      for (int j = 0; j < N; j++)
         os << m.entries[i][j] << " ";
   }
   return os;
}

#endif

