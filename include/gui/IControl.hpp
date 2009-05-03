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

#ifndef INC_GUI_ICONTROL_HPP
#define INC_GUI_ICONTROL_HPP

#include "IFont.hpp"

#include <tr1/memory>
#include <string>

#include <boost/cast.hpp>

namespace gui {

   // Interface to any UI control
   struct IControl {
      virtual ~IControl() {}

      // Draw the control and any children
      virtual void render(int x = 0, int y = 0) const = 0;
   };

   typedef std::tr1::shared_ptr<IControl> IControlPtr;

   // Interface to a UI control that contains text
   struct ITextControl : IControl {
      virtual ~ITextControl() {}
      
      virtual void setText(const std::string& aString) = 0;
      virtual void setText(const char* fmt, ...) = 0;
   };

   typedef std::tr1::shared_ptr<ITextControl> ITextControlPtr;

   // Standard controls
   IControlPtr makeButton(const std::string& aGlyphFile);
   ITextControlPtr makeLabel(IFontPtr aFont, const std::string& aString="");
}

#endif
