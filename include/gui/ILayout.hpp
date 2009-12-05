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

#ifndef INC_GUI_ILAYOUT_HPP
#define INC_GUI_ILAYOUT_HPP

#include "Platform.hpp"
#include "gui/Widget.hpp"

#include <string>

#include <boost/any.hpp>

namespace gui {
   
   // A complete set of UI elements
   struct ILayout {
      virtual ~ILayout() {}

      template <class T>
      T& cast(const string& path) const
      {
         return dynamic_cast<T&>(get(path));
      }

      virtual Widget& get(const string& path) const = 0;
      virtual void render() const = 0;

      virtual bool click(int x, int y) = 0;
   };

   typedef shared_ptr<ILayout> ILayoutPtr;

   ILayoutPtr makeLayout(const string& file_name);
   string parentPath(const string& path);
   
}

#endif
