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
#include "gui/RenderContext.hpp"
#include "IXMLParser.hpp"

#include <string>
#include <map>

namespace gui {

   class Widget {
   public:
      Widget(const AttributeSet& attrs);

      const string& name() const { return name_; }
      int x() const { return x_; }
      int y() const { return y_; }
      int width() const { return width_ + 2*border_; }
      int height() const { return height_ + 2*border_; }
      bool visible() const { return visible_; }
      int border() const { return border_; }

      void x(int x) { x_ = x; }
      void y(int y) { y_ = y; }
      void width(int w) { width_ = w; }
      void height(int h) { height_ = h; }
      void visible(bool v);

      enum Signal {
         SIG_CLICK, SIG_RENDER, SIG_SHOW, SIG_HIDE,
         SIG_ENTER, SIG_LEAVE
      };

      typedef function<void (Widget&)> SignalHandler;

      void connect(Signal sig, SignalHandler handler);

      virtual void render(RenderContext& rc) const = 0;
      virtual void adjust_for_theme(const Theme& theme) {}
      
      virtual bool handle_click(int x, int y);

      void dump_location() const;
      
   protected:
      void raise(Signal sig);

   private:
      static string unique_name();
      
      string name_;
      int x_, y_, width_, height_;
      bool visible_;
      int border_;

      map<Signal, SignalHandler> handlers;
            
      static int our_unique_id;
   };
 
}

#endif
