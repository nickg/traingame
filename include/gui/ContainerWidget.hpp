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

#ifndef INC_CONTAINER_WIDGET_HPP
#define INC_CONTAINER_WIDGET_HPP

#include "Platform.hpp"
#include "gui/Widget.hpp"

#include <vector>

namespace gui {

   // A widget which contains other widgets
   class ContainerWidget : public Widget {
   public:
      ContainerWidget(const AttributeSet& attrs);

      virtual void render(RenderContext& rc) const;
      virtual void adjust_for_theme(const Theme& theme);

      void add_child(Widget* w);
      
      virtual bool handle_click(int x, int y);
      
   protected:
      virtual void child_added(Widget* w) {};
      int count_children();

      typedef vector<Widget*> ChildList;
      
      ChildList::iterator begin() { return children.begin(); }
      ChildList::iterator end() { return children.end(); }

      ChildList::const_iterator const_begin() const
      { return children.begin(); }
      ChildList::const_iterator const_end() const
      { return children.end(); }
      
   private:
      ChildList children;
   };
   
}

#endif
