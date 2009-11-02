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

#ifndef INC_FT_IFONT_HPP
#define INC_FT_IFONT_HPP

#include "Platform.hpp"
#include "Colour.hpp"

namespace ft {
   
   struct IFont {
      virtual ~IFont() {}

      virtual void print(int x, int y, Colour c,
         const string& s) const = 0;

      virtual int height() const = 0;
      virtual int text_width(const string& s) const = 0;
   };

   typedef shared_ptr<IFont> IFontPtr;

   enum FontType {
      FONT_NORMAL, FONT_MONO
   };
   
   IFontPtr loadFont(const string& file, int h,
      FontType type=FONT_NORMAL, bool dropShadow=false);
   
}

#endif
