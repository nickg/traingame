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

#include "gui2/RenderContext.hpp"

#include <GL/gl.h>

using namespace gui;

namespace {
   inline void set_colour(Colour c)
   {
      glColor4f(get<0>(c), get<1>(c), get<2>(c), get<3>(c));
   }
}

RenderContext::RenderContext()
   : origin_x(0), origin_y(0)
{

}

RenderContext::~RenderContext()
{

}

void RenderContext::push_origin(int x, int y)
{
   origin_x = x;
   origin_y = y;
   origin_stack.push(make_pair(x, y));
}

void RenderContext::pop_origin()
{
   tie(origin_x, origin_y) = origin_stack.top();
   origin_stack.pop();
}

void RenderContext::offset(int& x, int& y) const
{
   x += origin_x;
   y += origin_y;
}

void RenderContext::rectangle(int x, int y, int w, int h, Colour c)
{
   offset(x, y);
   set_colour(c);
   
   glBegin(GL_QUADS);
   glVertex2i(x, y);
   glVertex2i(x + w, y);
   glVertex2i(x + w, y + h);
   glVertex2i(x, y + h);
   glEnd();
}

void RenderContext::border(int x, int y, int w, int h, Colour c)
{
   offset(x, y);
   set_colour(c);
   
   glBegin(GL_LINE_LOOP);
   glVertex2i(x, y);
   glVertex2i(x + w, y);
   glVertex2i(x + w, y + h);
   glVertex2i(x, y + h);
   glEnd();
}

void RenderContext::print(IFontPtr font, int x, int y, const string& s)
{
   offset(x, y);
   font->print(x, y, "%s", s.c_str());
}
