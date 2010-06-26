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

#include "gui/Widget.hpp"
#include "ILogger.hpp"
#include "GameScreens.hpp"

#include <boost/lexical_cast.hpp>

using namespace gui;

int Widget::our_unique_id(0);

Widget::Widget(const AttributeSet& attrs)
   : name_(attrs.get<string>("name", unique_name())),
     x_(attrs.get<int>("x", -1)),
     y_(attrs.get<int>("y", -1)),
     width_(attrs.get<int>("width", 0)),
     height_(attrs.get<int>("height", 0)),
     visible_(attrs.get<bool>("visible", true)),
     border_(attrs.get<int>("border", 0))
{
   // If x or y weren't specified center the widget in the screen
   if (x_ == -1 || y_ == -1) {
      const int screenW = get_game_window()->width();
      const int screenH = get_game_window()->height();

      if (x_ == -1)
         x_ = (screenW - width_) / 2;

      if (y_ == -1)
         y_ = (screenH - height_) / 2;
   }
}

string Widget::unique_name()
{
   return "widget" + boost::lexical_cast<string>(our_unique_id++);
}

void Widget::raise(Signal sig)
{
   map<Signal, SignalHandler>::iterator it = handlers.find(sig);
   if (it != handlers.end())
      (*it).second(*this);
}

void Widget::connect(Signal sig, SignalHandler handler)
{
   handlers[sig] = handler;
}

bool Widget::handle_click(int x, int y)
{
   raise(SIG_CLICK);
   return true;
}

void Widget::dump_location() const
{
   debug() << name() << ": x=" << x()
           << " y=" << y() << " width=" << width()
           << " height=" << height();
}

void Widget::visible(bool v)
{
   bool event = visible_ != v;
   
   visible_ = v;

   if (event) {
      if (visible_)
         raise(SIG_SHOW);
      else
         raise(SIG_HIDE);
   }
}
