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

#ifndef INC_GUI_WINDOW_HPP
#define INC_GUI_WINDOW_HPP

// Internal header: do not include this file directly

#include "Platform.hpp"
#include "gui/Widget.hpp"
#include "gui/ContainerWidget.hpp"

#include <string>

namespace gui {

   class Window : public ContainerWidget {
   public:
      Window(const AttributeSet& attrs);

      const string& title() const { return title_; }
      void title(const string& t) { title_ = t; }

      void render(RenderContext& rc) const;
      void adjustForTheme(const Theme& theme);
   private:
      string title_;
      bool dynamicWidth, dynamicHeight;
      int border;
   };
   
}

#endif
