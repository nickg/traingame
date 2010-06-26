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

#ifndef INC_GUI_THROTTLE_METER_HPP
#define INC_GUI_THROTTLE_METER_HPP

#include "Platform.hpp"
#include "gui/Widget.hpp"

#include <string>

namespace gui {

   class ThrottleMeter : public Widget {
   public:
      ThrottleMeter(const AttributeSet& attrs);

      void value(int v) { value_ = v; }
      int value() const { return value_; }

      void range(int low, int high);
      
      void render(RenderContext& rc) const;
      void adjust_for_theme(const Theme& theme);

   private:
      int value_, min_value, max_value;
      string font_name;
      
      static const int THROTTLE_MAX = 10;
      static const int THROTTLE_MIN = 0;
      
      static const int METER_HEIGHT, METER_WIDTH;
   };
   
}

#endif
