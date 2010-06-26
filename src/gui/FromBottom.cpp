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

#include <cassert>

using namespace gui;

FromBottom::FromBottom(const AttributeSet& attrs)
   : ContainerWidget(attrs),
     offset(attrs.get<int>("offset"))
{
   // It would be nice if this worked inside other widgets
   int sh = get_game_window()->height();
   int sw = get_game_window()->width();

   x(0);
   width(sw);
   height(offset);
   y(sh - offset);

   dump_location();
}

void FromBottom::render(RenderContext& rc) const
{   
   rc.push_origin(this);
   ContainerWidget::render(rc);
   rc.pop_origin();
}

