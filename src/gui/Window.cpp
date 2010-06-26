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

#include "gui/Window.hpp"
#include "ILogger.hpp"

using namespace gui;

Window::Window(const AttributeSet& attrs)
   : ContainerWidget(attrs),
     title_(attrs.get<string>("title", ""))
{
   dynamic_width = !attrs.has("width");
   dynamic_height = !attrs.has("height");

   if (dynamic_width)
      width(0);
   if (dynamic_height)
      height(0);
}

void Window::render(RenderContext& rc) const
{   
   rc.rectangle(x(), y(), width(), height(),
      rc.theme().background());
   rc.border(x(), y(), width(), height(),
      rc.theme().border());

   rc.push_origin(this);

   ContainerWidget::render(rc);
   
   rc.pop_origin();
}

void Window::adjust_for_theme(const Theme& theme)
{
   ContainerWidget::adjust_for_theme(theme);
   
   if (dynamic_width) {
      int maxW = 0;
      for (ChildList::const_iterator it = const_begin();
           it != const_end(); ++it) {
         int w = (*it)->width();
         int x = (*it)->x();

         if (x + w > maxW)
            maxW = x + w;
      }

      width(maxW);
   }

   if (dynamic_height) {
      int maxH = 0;
      for (ChildList::const_iterator it = const_begin();
           it != const_end(); ++it) {
         int h = (*it)->height();
         int y = (*it)->y();

         if (y + h > maxH)
            maxH = y + h;
      }

      height(maxH);
   }   
}
