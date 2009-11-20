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

#include "gui2/ThrottleMeter.hpp"

#include <stdexcept>
#include <algorithm>

#include <GL/gl.h>

using namespace gui;

const int ThrottleMeter::METER_HEIGHT(16);
const int ThrottleMeter::METER_WIDTH(100);

ThrottleMeter::ThrottleMeter(const AttributeSet& attrs)
   : Widget(attrs),
     value_(0),
     minValue(THROTTLE_MIN),
     maxValue(THROTTLE_MAX),
     fontName(attrs.get<string>("font", ""))
{

}

void ThrottleMeter::range(int low, int high)
{
   minValue = low;
   maxValue = high;
}

void ThrottleMeter::render(RenderContext& rc) const
{
   IFontPtr font = rc.theme().font(fontName);
   
   int ox = x(), oy = y();
   rc.offset(ox, oy);

   font->print(ox, oy, colour::WHITE, "Throttle: ");

   glPushMatrix();

   const int off = (height() - METER_HEIGHT) / 2;

   glTranslatef(ox + static_cast<float>(font->text_width("Throttle: ")),
                static_cast<float>(oy + off), 0.0f);

   const int unit = METER_WIDTH / (maxValue + 1);

   // Neutral bit
   glColor3f(1.0f, 1.0f, 0.0f);
   glBegin(GL_QUADS);
   glVertex2i(0, 0);
   glVertex2i(0, METER_HEIGHT);
   glVertex2i(unit, METER_HEIGHT);
   glVertex2i(unit, 0);
   glEnd();

   int squareLen = value_ >= maxValue
      ? (maxValue - 1) * unit
      : (value_ > 0 ? unit * (value_ - 1) : 0);

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
   
   const bool wantTriangle = value_ < maxValue && value_ > 0;
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

void ThrottleMeter::adjustForTheme(const Theme& theme)
{
   IFontPtr font = theme.font(fontName);

   width(font->text_width("Throttle: ") + METER_WIDTH);
   height(max(font->height(), METER_HEIGHT));
}
