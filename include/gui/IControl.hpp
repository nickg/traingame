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
#include <tr1/tuple>
#include <string>

#include <boost/cast.hpp>

namespace gui {

   typedef std::tr1::tuple<float, float, float> Colour;

   // Interface to any UI control
   struct IControl {
      virtual ~IControl() {}

      // Draw the control and any children
      virtual void render(int x = 0, int y = 0) const = 0;

      // Return the dimensions of the control
      virtual int width() const = 0;
      virtual int height() const = 0;

      // Show / hide the control
      virtual void setVisible(bool visible) = 0;
   };

   typedef std::tr1::shared_ptr<IControl> IControlPtr;

   // Interface to a UI control that contains text
   struct ITextControl : IControl {
      virtual ~ITextControl() {}
      
      virtual void setText(const std::string& aString) = 0;
      virtual void setText(const char* fmt, ...) = 0;

      virtual void setColour(float r, float g, float b) = 0;
   };

   typedef std::tr1::shared_ptr<ITextControl> ITextControlPtr;

   // Interface to controls that are progress meters, etc.
   struct IMeterControl : IControl {
      virtual ~IMeterControl() {}

      virtual void setValue(int aValue) = 0;
      virtual void setRange(int aLowValue, int aHighValue) = 0;
   };

   typedef std::tr1::shared_ptr<IMeterControl> IMeterControlPtr;

   // Standard controls
   IControlPtr makeButton(const std::string& aGlyphFile);
   ITextControlPtr makeLabel(IFontPtr aFont, const std::string& aString="");
   IMeterControlPtr makeThrottleMeter(IFontPtr aFont);
   IMeterControlPtr makeFuelMeter(IFontPtr aFont, const std::string& aCaption,
                                  const Colour& aColour);
}

#endif
