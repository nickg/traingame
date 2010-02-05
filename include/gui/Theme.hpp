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

#ifndef INC_GUI_THEME_HPP
#define INC_GUI_THEME_HPP

// Internal header: do not include this file directly

#include "Platform.hpp"
#include "Colour.hpp"
#include "IFont.hpp"

#include <map>

namespace gui {
   
    class Theme {
    public:
	Theme();

	// Colours
	Colour background() const;
	Colour border() const;

	// Fonts
	IFontPtr normalFont() const { return normal_font_; }
	IFontPtr font(const string& fontName) const;

	void addFont(const string& name, IFontPtr f);
      
    private:
	IFontPtr normal_font_;

	typedef map<string, IFontPtr> FontMap;
	FontMap fonts;
    };
   
}

#endif
