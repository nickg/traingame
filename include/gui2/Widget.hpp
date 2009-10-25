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
#include "gui2/RenderContext.hpp"
#include "IXMLParser.hpp"

#include <string>
#include <map>

namespace gui {

   class Widget : public IWidget {
   public:
      Widget(const AttributeSet& attrs);

      const string& name() const { return name_; }
      int x() const { return x_; }
      int y() const { return y_; }
      int width() const { return width_; }
      int height() const { return height_; }
      
      void width(int w) { width_ = w; }
      void height(int h) { height_ = h; }

      boost::any get_property(const string& key) const;
      void set_property(const string& key, boost::any value);

      virtual void render(RenderContext& rc) const = 0;

   private:
      static string unique_name();
      
      string name_;
      int x_, y_, width_, height_;
            
      static int unique_id;
   };
 
}

#endif
