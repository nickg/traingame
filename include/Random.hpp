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

#ifndef INC_RANDOM_HPP
#define INC_RANDOM_HPP

#include "Platform.hpp"

#include <ctime>
#include <cstdlib>

#include <boost/random.hpp>

// Some useful wrappers around the complexities of Boost's
// random number classes

namespace distribution {

   // Template trickery to pick the right distribution for a type

   template <typename T>
   struct UniformFor;
   
   template <>
   struct UniformFor<float> { typedef boost::uniform_real<float> U; };

   template <>
   struct UniformFor<int> { typedef boost::uniform_int<> U; };
   
   template <typename T>
   struct NormalFor { typedef boost::normal_distribution<T> N; };
   
}

template <typename T>
struct Uniform {
   typedef typename distribution::UniformFor<T>::U Distribution;
   
   Uniform(T min, T max)
      : rnd(boost::mt19937(static_cast<uint32_t>(time(NULL))),
         Distribution(min, max))
   {
   }

   T operator()()
   {
      return rnd();
   }
   
   boost::variate_generator<boost::mt19937, Distribution> rnd;
};

template <typename T>
struct Normal {
   typedef typename distribution::NormalFor<T>::N Distribution;
   
   Normal(T mean, T std)
      : rnd(boost::mt19937(static_cast<uint32_t>(time(NULL))), 
         Distribution(mean, std))
   {
   }

   T operator()()
   {
      return rnd();
   }

   boost::variate_generator<boost::mt19937, Distribution> rnd;
};

#endif
