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

#include "gui2/Button.hpp"

using namespace gui;

Button::Button(const AttributeSet& attrs)
   : Widget(attrs),
     label_(attrs.get<string>("label"))
{
   
}

void Button::render(RenderContext& rc) const
{
   rc.rectangle(x(), y(), width(), height(),
      rc.theme().background());

   IFontPtr f = rc.theme().normal_font();

   int center = (height() - f->height()) / 2;
   
   rc.print(f, x() + 5, y() + center, label_);

   rc.border(x(), y(), width(), height(),
      rc.theme().border());
}
