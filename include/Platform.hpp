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

#ifndef INC_PLATFORM_HPP
#define INC_PLATFORM_HPP

//
// A collection of hacks to get everything building smoothly across
// a variety of platforms
//

// First, make sure we get a decent TR1 implementation
#if defined __GNUC__

#include <tr1/memory>
#include <tr1/tuple>
#include <tr1/functional>

#elif (_MSVC_VER >= 1500)

// MSVC9 has TR1 available as an add-on pack

#include <memory>
#include <tuple>
#include <functional>

#else

// See if the Boost implementation is available

#include <boost/tr1/memory.hpp>
#include <boost/tr1/tuple.hpp>
#include <boost/tr1/functional.hpp>

#endif

using namespace std;
using namespace std::tr1;

#endif
