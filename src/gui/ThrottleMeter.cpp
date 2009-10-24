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
#include "gui/Internal.hpp"

#include <stdexcept>

#include <GL/gl.h>

using namespace std;
using namespace gui;

class ThrottleMeter : public IMeterControl {
public:
   ThrottleMeter(IFontPtr aFont);
   ~ThrottleMeter() {}

   // IControl interface
   int width() const;
   int height() const;
   void render(int x, int y) const;

   // IMeterControl interface
   void setValue(int aValue);
   void setRange(int aLowValue, int aHighValue);
private:
   int myValue;
   IFontPtr myFont;
   const int myTextWidth;
   int myMin, myMax;

   static const int THROTTLE_MAX = 10;
   static const int THROTTLE_MIN = 0;

   static const int METER_HEIGHT, METER_WIDTH;
};

const int ThrottleMeter::METER_HEIGHT(16);
const int ThrottleMeter::METER_WIDTH(100);

ThrottleMeter::ThrottleMeter(IFontPtr aFont)
   : myValue(0), myFont(aFont),
     myTextWidth(myFont->string_width("Throttle: ")),
     myMin(THROTTLE_MIN), myMax(THROTTLE_MAX)
{
   
}

int ThrottleMeter::height() const
{
   return max(myFont->max_height(), METER_HEIGHT);
}

int ThrottleMeter::width() const
{
   return myTextWidth + METER_WIDTH;
}

void ThrottleMeter::setValue(int aValue)
{
   myValue = aValue;
}

void ThrottleMeter::setRange(int aLowValue, int aHighValue)
{
   myMin = aLowValue;
   myMax = aHighValue;
}

void ThrottleMeter::render(int x, int y) const
{   
   myFont->print(x, y, "Throttle: ");

   glPushMatrix();

   const int off = height() - METER_HEIGHT + 1;

   glTranslatef(static_cast<float>(myTextWidth),
                static_cast<float>(y + off), 0.0f);

   const int unit = METER_WIDTH / (myMax + 1);

   // Neutral bit
   glColor3f(1.0f, 1.0f, 0.0f);
   glBegin(GL_QUADS);
   glVertex2i(0, 0);
   glVertex2i(0, METER_HEIGHT);
   glVertex2i(unit, METER_HEIGHT);
   glVertex2i(unit, 0);
   glEnd();

   int squareLen = myValue >= myMax
      ? (myMax - 1) * unit
      : (myValue > 0 ? unit * (myValue - 1) : 0);

   glTranslatef(static_cast<float>(unit), 0.0f, 0.0f);
   glColor3f(0.0f, 1.0f, 0.0f);
   
   // Forwards bit
   if (squareLen > 0) {
      glBegin(GL_QUADS);
      glVertex2i(0, 0);
      glVertex2i(0, METER_HEIGHT);
      glVertex2i(squareLen, METER_HEIGHT);
      glVertex2i(squareLen, 0);
      glEnd();
   }
   
   const bool wantTriangle = myValue < myMax && myValue > 0;
   if (wantTriangle) {
      // Triangle bit
      glBegin(GL_TRIANGLES);
      glVertex2i(squareLen, 0);
      glVertex2i(squareLen, METER_HEIGHT);
      glVertex2i(squareLen + unit, METER_HEIGHT / 2);
      glEnd();
   }   
   
   glPopMatrix();
}

IMeterControlPtr gui::makeThrottleMeter(IFontPtr aFont)
{
   return IMeterControlPtr
      (new Defaults<Moveable<Hideable<ThrottleMeter> > >(aFont));
}
