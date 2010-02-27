//
//  Copyright (C) 2009-2010  Nick Gasson
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

#ifndef INC_ISCENERYPICKER_HPP
#define INC_ISCENERYPICKER_HPP

#include "Platform.hpp"
#include "gui/ILayout.hpp"
#include "IScenery.hpp"

// A dialog box for picking scenery in the editor
struct ISceneryPicker {
   virtual ~ISceneryPicker() {}

   virtual ISceneryPtr get() const = 0;
};

typedef shared_ptr<ISceneryPicker> ISceneryPickerPtr;

ISceneryPickerPtr makeSceneryPicker(gui::ILayoutPtr layout);

#endif
