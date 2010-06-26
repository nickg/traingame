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

#include "gui/Theme.hpp"
#include "ILogger.hpp"

#include <string>
#include <stdexcept>

using namespace gui;

Theme::Theme()
{
   normal_font_ = gui::load_font("fonts/DejaVuSans.ttf",
      17, gui::FONT_NORMAL);
}

void Theme::add_font(const string& name, IFontPtr f)
{
   fonts[name] = f;
}

Colour Theme::background() const
{
   return make_colour(0.0f, 0.0f, 0.3f, 0.5f);
}

Colour Theme::border() const
{
   return make_colour(0.0f, 0.0f, 1.0f, 1.0f);
}

IFontPtr Theme::font(const string& font_name) const
{
   if (font_name == "")
      return normal_font();
   else {
      FontMap::const_iterator it = fonts.find(font_name);
      if (it != fonts.end())
         return (*it).second;
      else 
         throw runtime_error("Unknown font: " + font_name);
   }
}

