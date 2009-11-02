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
#include "gui2/Widget.hpp"
#include "ILogger.hpp"
#include "IWindow.hpp"
#include "OpenGLHelper.hpp"

#include <GL/gl.h>

using namespace gui;

IWindowPtr getGameWindow();

RenderContext::RenderContext()
   : origin_x(0), origin_y(0)
{
   glPushAttrib(GL_ENABLE_BIT);
   glEnable(GL_SCISSOR_TEST);
}

RenderContext::~RenderContext()
{
   glPopAttrib();

   IWindowPtr wnd = getGameWindow();
   glScissor(0, 0, wnd->width(), wnd->height());
}

void RenderContext::push_origin(const Widget* w)
{
   origin_x = w->x();
   origin_y = w->y();
   origin_stack.push(w);
}

void RenderContext::pop_origin()
{
   origin_x = origin_stack.top()->x();
   origin_y = origin_stack.top()->y();
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
   gl::colour(c);
   
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
   gl::colour(c);
   
   glBegin(GL_LINE_LOOP);
   glVertex2f(x + 0.1f, y + 0.1f);
   glVertex2f(x + w - 0.1f, y + 0.1f);
   glVertex2f(x + w - 0.1f, y + h - 0.1f);
   glVertex2f(x + 0.1f, y + h - 0.1f);
   glEnd();
}

void RenderContext::print(IFontPtr font, int x, int y, const string& s)
{
   offset(x, y);
   font->print(x, y, makeColour(1.0f, 1.0f, 1.0f), s);
}

void RenderContext::scissor(Widget* w)
{
   int wh = getGameWindow()->height();

   const Widget* parent = origin_stack.empty() ? NULL : origin_stack.top();
   int max_w, max_h;
   if (parent) {
      max_w = parent->width() - w->x();
      max_h = parent->height() - w->y();
   }
   else
      max_w = max_h = 100000;
   
   int x = w->x() - 1;
   int y = w->y() - 1;
   offset(x, y);

   y = wh - y - w->height() - 1;
   
   int width = min(w->width() + 1, max_w);
   int height = min(w->height(), max_h);
   
   glScissor(x, y, width, height);
}
