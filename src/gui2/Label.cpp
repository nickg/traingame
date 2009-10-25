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

#include "gui2/Label.hpp"
#include "ILogger.hpp"

using namespace gui;

Label::Label(const AttributeSet& attrs)
   : Widget(attrs)
{
   property("text", text_);
}

void Label::render(RenderContext& rc) const
{
   debug() << "Label::render x=" << x() << " y=" << y();
   rc.print(rc.theme().normal_font(), x(), y(), text_);
}
