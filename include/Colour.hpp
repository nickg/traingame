//
//  Copyright (C) 2009-2010  Nick Gasson
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

#ifndef INC_GUI_COLOUR_HPP
#define INC_GUI_COLOUR_HPP

#include "Platform.hpp"

struct Colour {
   float r, g, b, a;
};

inline Colour make_colour(float r, float g, float b, float a=1.0f)
{
   Colour c = { r, g, b, a };
   return c;
}

inline Colour make_rgb(int r, int g, int b, int a=255)
{
    return make_colour(
	static_cast<float>(r) / 255.0f,
	static_cast<float>(g) / 255.0f,
	static_cast<float>(b) / 255.0f,
	static_cast<float>(a) / 255.0f);
}

namespace colour {
   const Colour WHITE = make_colour(1.0f, 1.0f, 1.0f);
}

#endif
