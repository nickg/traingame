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
     buttonWidth(32), buttonHeight(32)
{
   width(1);
   height(buttonHeight);
}

void ToggleBar::render(RenderContext& rc) const
{
   rc.pushOrigin(this);
   ContainerWidget::render(rc);
   rc.popOrigin();
}

void ToggleBar::childAdded(Widget* w)
{
   debug() << "Added " << w->name() << " to toggle bar";

   w->x(nextX);
   w->y(0);
   w->width(buttonWidth);
   w->height(buttonHeight);

   nextX += buttonWidth;

   width(width() + buttonWidth);

   if (countChildren() == 1)
      boost::polymorphic_cast<ToggleButton*>(w)->on();
}

bool ToggleBar::handleClick(int x, int y)
{
   ChildList::iterator it;
   for (it = begin(); it != end(); ++it)
      boost::polymorphic_cast<ToggleButton*>(*it)->off();
   
   return ContainerWidget::handleClick(x, y);
}
