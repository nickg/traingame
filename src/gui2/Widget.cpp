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

#ifndef INC_WIDGET_HPP
#define INC_WIDGET_HPP

#include "gui2/Widget.hpp"

using namespace gui;

int Widget::unique_id(0);

Widget::Widget(const AttributeSet& attrs)
   : tmp_attrs(attrs)
{
   const_property("name", name_, unique_name());
}

string Widget::unique_name()
{
   return "widget" + unique_id++;
}

#endif
