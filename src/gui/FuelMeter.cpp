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
#include <boost/lexical_cast.hpp>

using namespace gui;
using namespace std;
using namespace std::tr1;
using namespace boost;

class FuelMeter : public IMeterControl, private ControlImpl {
public:
   FuelMeter(IFontPtr aFont, const string& aCaption,
             const Colour& aColour);
   ~FuelMeter() {}

   // IControl interface   
   void renderVisible(int x, int y) const;
   int width() const;
   int height() const;

   // IMeterControl interface
   void setValue(int aValue);
   void setRange(int aLowValue, int aHighValue);
private:
   int myValue;
   IFontPtr myFont;
   const string myCaption;
   const Colour myColour;
   const int myTextWidth;
   int myMin, myMax;

   static const int METER_HEIGHT, METER_WIDTH;
};

const int FuelMeter::METER_HEIGHT(16);
const int FuelMeter::METER_WIDTH(100);

FuelMeter::FuelMeter(IFontPtr aFont, const string& aCaption,
                     const Colour& aColour)
   : myValue(0), myFont(aFont), myCaption(aCaption + ": "),
     myColour(aColour),
     myTextWidth(myFont->stringWidth(myCaption.c_str())),
     myMin(0), myMax(10)
{
   
}

int FuelMeter::height() const
{
   return max(myFont->maxHeight(), METER_HEIGHT);
}

int FuelMeter::width() const
{
   return myTextWidth + METER_WIDTH;
}

void FuelMeter::setValue(int aValue)
{
   if (aValue < myMin)
      throw runtime_error("Fuel meter underflow: "
                          + lexical_cast<string>(aValue));
   else if (aValue > myMax)
      throw runtime_error("Fuel meter overflow: "
                          + lexical_cast<string>(aValue));
   
   myValue = aValue;
}

void FuelMeter::setRange(int aLowValue, int aHighValue)
{
   myMin = aLowValue;
   myMax = aHighValue;
}

void FuelMeter::renderVisible(int x, int y) const
{
   myFont->print(x, y, myCaption.c_str());

   glPushMatrix();

   const int off = height() - METER_HEIGHT + 1;
   
   glTranslatef(static_cast<float>(myTextWidth),
                static_cast<float>(y + off), 0.0f);

   const float unit = METER_WIDTH / static_cast<float>(myMax + 1);

   glColor3f(get<0>(myColour), get<1>(myColour), get<2>(myColour));
   glBegin(GL_QUADS);
   glVertex2i(0, 0);
   glVertex2i(0, METER_HEIGHT);
   glVertex2f(unit * myValue, METER_HEIGHT);
   glVertex2f(unit * myValue, 0);
   glEnd();

   glPopMatrix();
}

IMeterControlPtr gui::makeFuelMeter(IFontPtr aFont, const string& aCaption,
                                    const Colour& aColour)
{
   return IMeterControlPtr(new FuelMeter(aFont, aCaption, aColour));
}
