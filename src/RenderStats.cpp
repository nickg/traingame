//
//  Copyright (C) 2010  Nick Gasson
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

#include "IRenderStats.hpp"
#include "GameScreens.hpp"
#include "IMesh.hpp"

#include <boost/lexical_cast.hpp>

class RenderStats : public IRenderStats {
public:
   RenderStats(gui::ILayoutPtr layout, const string& path);

   // IRenderStats interface
   void update(int delta);
   
private:
   int ticks_until_update;
   gui::ILayoutPtr layout;   // Hold to ensure validity of label reference
   gui::Label& label;
};

RenderStats::RenderStats(gui::ILayoutPtr layout, const string& path)
   : ticks_until_update(0),
     layout(layout),
     label(layout->cast<gui::Label>(path))
{

}

void RenderStats::update(int delta)
{
   ticks_until_update -= delta;
   
   if (ticks_until_update <= 0) {
      int avg_triangles = get_average_triangleCount();
      
      label.text(
         "FPS: " + boost::lexical_cast<string>(get_game_window()->get_fps())
         + " [" + boost::lexical_cast<string>(avg_triangles) + " triangles]");

      ticks_until_update = 1000;
   }
}

IRenderStatsPtr make_render_stats(gui::ILayoutPtr layout, const string& path)
{
   return IRenderStatsPtr(new RenderStats(layout, path));
}

