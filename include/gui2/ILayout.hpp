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

#include <string>

#include <boost/any.hpp>

namespace gui {
   
   // A generic UI element
   struct IWidget {
      virtual ~IWidget() {}

      virtual boost::any get_property(const string& key) const = 0;
      virtual void set_property(const string& key, boost::any value) = 0;
   };

   typedef shared_ptr<IWidget> IWidgetPtr;

   template <typename T>
   inline T get_property(IWidgetPtr elem, const string& key)
   {
      return boost::any_cast<T>(elem->get_property(key));
   }

   template <typename T>
   inline void set_property(IWidgetPtr elem, const string& key,
      const T& value)
   {
      elem->set_property(key, value);
   }
   
   // A complete set of UI elements
   struct ILayout {
      virtual ~ILayout() {}

      virtual IWidgetPtr get(const string& path) const = 0;
      virtual void render() const = 0;
   };

   typedef shared_ptr<ILayout> ILayoutPtr;

   ILayoutPtr make_layout(const string& file_name);

}

#endif
