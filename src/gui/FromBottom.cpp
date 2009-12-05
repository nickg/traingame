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

#include "gui/FromBottom.hpp"
#include "GameScreens.hpp"

using namespace gui;

FromBottom::FromBottom(const AttributeSet& attrs)
   : ContainerWidget(attrs),
     offset(attrs.get<int>("offset"))
{
   // It would be nice if this worked inside other widgets
   int sh = getGameWindow()->height();
   int sw = getGameWindow()->width();

   x(0);
   width(sw);
   height(offset);
   y(sh - offset);

   dumpLocation();
}

void FromBottom::render(RenderContext& rc) const
{
   rc.pushOrigin(this);
   ContainerWidget::render(rc);
   rc.popOrigin();
}

