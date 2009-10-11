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

#ifndef INC_GUI_WIDGET_HPP
#define INC_GUI_WIDGET_HPP

// Internal header: do not include this file directly

#include "Platform.hpp"
#include "gui2/ILayout.hpp"
#include "IXMLParser.hpp"

#include <string>
#include <map>

#include <boost/any.hpp>

namespace gui {

   class Widget : public IWidget {
   public:
      Widget(const AttributeSet& attrs);

      const string& name() const { return name_; }

   protected:      
      template <class T>
      void const_property(const string& key, T& value,
         const T& def = T())
      {
         value = tmp_attrs.get<T>(key, def);
         read_properties[key] = value;
      }

      template <class T>
      void property(const string& key, T& value, const T& def = T())
      {
         const_property(key, value);   // Readable as well
         write_properties[key] = ref(value);
      }
      
   private:
      static string unique_name();
      
      string name_;
      const AttributeSet& tmp_attrs;   // Do not use after initialisation
      
      map<string, boost::any> read_properties, write_properties;

      static int unique_id;
   };
 
}

#endif
