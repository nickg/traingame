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

#include "gui2/ContainerWidget.hpp"

using namespace gui;

ContainerWidget::ContainerWidget(const AttributeSet& attrs)
   : Widget(attrs)
{

}

void ContainerWidget::render(RenderContext& rc) const
{
   for (ChildList::const_iterator it = const_begin();
        it != const_end(); ++it)
      (*it)->render(rc);
}

void ContainerWidget::add_child(Widget* w)
{
   children.push_back(w);
   child_added(w);
}
