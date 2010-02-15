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
     title_(attrs.get<string>("title", "")),
     border(attrs.get<int>("border", 0))
{
   dynamicWidth = width() == -1;
   dynamicHeight = height() == -1;

   if (dynamicWidth)
      width(0);
   if (dynamicHeight)
      height(0);
}

void Window::render(RenderContext& rc) const
{   
   rc.rectangle(x(), y(), width(), height(),
      rc.theme().background());
   rc.border(x(), y(), width(), height(),
      rc.theme().border());

   rc.pushOrigin(this, border, border);

   ContainerWidget::render(rc);
   
   rc.popOrigin();
}

void Window::adjustForTheme(const Theme& theme)
{
   ContainerWidget::adjustForTheme(theme);
   
   if (dynamicWidth) {
      int maxW = 0;
      for (ChildList::const_iterator it = constBegin();
           it != constEnd(); ++it) {
         int w = (*it)->width();
         int x = (*it)->x();

         if (x + w > maxW)
            maxW = x + w;
      }

      width(maxW + 2*border);
   }

   if (dynamicHeight) {
      int maxH = 0;
      for (ChildList::const_iterator it = constBegin();
           it != constEnd(); ++it) {
         int h = (*it)->height();
         int y = (*it)->y();

         if (y + h > maxH)
            maxH = y + h;
      }

      height(maxH + 2*border);
   }   
}
