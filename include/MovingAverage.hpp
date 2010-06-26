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

#ifndef INC_MOVING_AVERAGE_HPP
#define INC_MOVING_AVERAGE_HPP

// Generic class to compute moving average of last N values
template <class T, int N>
class MovingAverage {
public:
   MovingAverage()
   {
      for (int i = 0; i < N; i++)
         my_samples[i] = static_cast<T>(0);
   }

   T value() const
   {
      T sum = static_cast<T>(0);
      for (int i = 0; i < N; i++)
         sum += my_samples[i];
      
      return sum / static_cast<T>(N);
   }

   void operator<<(T a_value)
   {
      for (int i = N - 1; i > 0; i--)
         my_samples[i] = my_samples[i-1];
      my_samples[0] = a_value;
   }
   
private:
   T my_samples[N];
};

#endif
