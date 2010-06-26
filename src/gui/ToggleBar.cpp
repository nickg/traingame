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

#include "gui/ToggleBar.hpp"
#include "gui/ToggleButton.hpp"
#include "ILogger.hpp"

#include <boost/cast.hpp>

using namespace gui;

ToggleBar::ToggleBar(const AttributeSet& attrs)
   : ContainerWidget(attrs),
     nextX(0),
     button_width(32), button_height(32)
{
   width(1);
   height(button_height);
}

void ToggleBar::render(RenderContext& rc) const
{
   rc.push_origin(this);
   ContainerWidget::render(rc);
   rc.pop_origin();
}

void ToggleBar::child_added(Widget* w)
{
   debug() << "Added " << w->name() << " to toggle bar";

   w->x(nextX);
   w->y(0);
   w->width(button_width);
   w->height(button_height);

   nextX += button_width;

   width(width() + button_width);

   if (count_children() == 1)
      boost::polymorphic_cast<ToggleButton*>(w)->on();
}

bool ToggleBar::handle_click(int x, int y)
{
   ChildList::iterator it;
   for (it = begin(); it != end(); ++it)
      boost::polymorphic_cast<ToggleButton*>(*it)->off();
   
   return ContainerWidget::handle_click(x, y);
}
