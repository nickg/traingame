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
     font_name(attrs.get<string>("font", "")),
     colour_(attrs.get<Colour>("colour", colour::WHITE)),
     dirty(true)
{
   
}

void Label::text(const string& t)
{
   if (text_ != t) {
      text_ = t;
      dirty = true;
   }
}

void Label::render(RenderContext& rc) const
{
   rc.print(rc.theme().font(font_name), x(), y(), text_, colour_);
}

void Label::adjust_for_theme(const Theme& theme)
{
   IFontPtr font = theme.font(font_name);

   if (dirty) {
      width(font->text_width(text_));
      height(font->height());

      dirty = false;
   }
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

