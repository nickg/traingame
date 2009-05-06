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

#ifndef INC_IFONT_HPP
#define INC_IFONT_HPP

#include <tr1/memory>
#include <string>

namespace gui {

   // Wrapper for FreeType fonts
   struct IFont {
      virtual ~IFont() {}

      virtual void setColour(float r, float g, float b, float a) = 0;
      virtual void getColour(float& r, float& g, float& b, float& a) const = 0;
      virtual void print(int x, int y, const char* fmt, ...) const = 0;
      virtual int stringWidth(const char* fmt, ...) const = 0;
      virtual int maxHeight() const = 0;
   };

   typedef std::tr1::shared_ptr<IFont> IFontPtr;

   IFontPtr loadFont(const std::string& aFile, int aHeight, bool shadow=true);
}

#endif
