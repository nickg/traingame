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

#include "gui/IControl.hpp"

#include <stdexcept>

#include <GL/gl.h>

using namespace std;
using namespace gui;

class ThrottleMeter : public IMeterControl {
public:
   ThrottleMeter(IFontPtr aFont);
   ~ThrottleMeter() {}

   // IControl interface
   void render(int x, int y) const;
   int width() const;
   int height() const;

   // IMeterControl interface
   void setValue(int aValue);
private:
   int myValue;
   IFontPtr myFont;

   static const int THROTTLE_MAX = 10;
   static const int THROTTLE_MIN = 0;

   static const int METER_HEIGHT = 20;
   static const int METER_WIDTH = 50;
};

ThrottleMeter::ThrottleMeter(IFontPtr aFont)
   : myValue(0), myFont(aFont)
{

}

int ThrottleMeter::height() const
{
   return max(myFont->maxHeight(), METER_HEIGHT);
}

int ThrottleMeter::width() const
{
   static const int textWidth = myFont->stringWidth("Throttle: ");
   
   return textWidth + METER_WIDTH;
}

void ThrottleMeter::setValue(int aValue)
{
   myValue = aValue;
}

void ThrottleMeter::render(int x, int y) const
{
   myFont->print(x, y, "Throttle: ");
}

IMeterControlPtr gui::makeThrottleMeter(IFontPtr aFont)
{
   return IMeterControlPtr(new ThrottleMeter(aFont));
}
