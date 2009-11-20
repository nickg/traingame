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

#include "gui/Label.hpp"
#include "ILogger.hpp"

#include <cstdarg>
#include <cstdio>

using namespace gui;

Label::Label(const AttributeSet& attrs)
   : Widget(attrs),
     text_(attrs.get<string>("text")),
     fontName(attrs.get<string>("font", "")),
     colour_(attrs.get<Colour>("colour", colour::WHITE))
{
   
}

void Label::render(RenderContext& rc) const
{
   rc.print(rc.theme().font(fontName), x(), y(), text_, colour_);
}

void Label::adjustForTheme(const Theme& theme)
{
   IFontPtr font = theme.font(fontName);
   
   width(font->text_width(text_));
   height(font->height());
}

void Label::format(const char* fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);

   char* buf;
   vasprintf(&buf, fmt, ap);
   text(buf);
   
   free(buf);
   va_end(ap);
}

