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
   
}

void Window::render(RenderContext& rc) const
{   
   rc.rectangle(x(), y(), width(), height(),
      rc.theme().background());
   rc.border(x(), y(), width(), height(),
      rc.theme().border());

   rc.pushOrigin(this);

   ContainerWidget::render(rc);
   
   rc.popOrigin();
}

