//
//  Copyright (C) 2009-2010  Nick Gasson
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

#include "GameScreens.hpp"
#include "ILogger.hpp"
#include "IModel.hpp"
#include "IMap.hpp"
#include "Maths.hpp"
#include "ILight.hpp"
#include "ISceneryPicker.hpp"
#include "Random.hpp"
#include "IRenderStats.hpp"

#include "gui/ILayout.hpp"
#include "gui/Label.hpp"

#include <algorithm>
#include <stdexcept>

#include <GL/gl.h>

// Concrete editor class
class Editor : public IScreen {
public:
   Editor(IMapPtr a_map);
   Editor(const string& a_map_name);
   ~Editor();
   
   void display(IGraphicsPtr a_context) const;
   void overlay() const;
   void update(IPickBufferPtr pick_buffer, int a_delta);
   void on_key_down(SDLKey a_key);
   void on_key_up(SDLKey a_key);
   void on_mouse_move(IPickBufferPtr pick_buffer,
      int x, int y, int xrel, int yrel);
   void on_mouse_click(IPickBufferPtr pick_buffer, int x, int y,
      MouseButton a_button);
   void on_mouse_release(IPickBufferPtr pick_buffer, int x, int y,
      MouseButton a_button);
   
   // Different tools the user can be using
   enum Tool {
      TRACK_TOOL, RAISE_TOOL, LOWER_TOOL, DELETE_TOOL,
      LEVEL_TOOL, START_TOOL, STATION_TOOL, BUILDING_TOOL,
      TREE_TOOL, SMOOTH_TOOL
   };
   void set_tool(Tool a_tool) { my_tool = a_tool; }

   IMapPtr get_map() { return map; }
   void set_map(IMapPtr a_map);
   
private:
   void build_gui();
   void draw_dragged_track();
   bool draw_track_tile(Point<int> where, track::Direction axis);
   void draw_dragged_straight(const track::Direction& an_axis, int a_length);
   void draw_dragged_curve(int x_length, int y_length);
   bool can_connect(const Point<int>& a_first_point,
      const Point<int>& a_second_point) const;
   bool can_place_track(ITrackSegmentPtr track);
   void drag_box_bounds(int& x_min, int& x_max, int &y_min, int& y_max) const;
   void delete_objects();
   void plant_trees();
   void save();
      
   IMapPtr map;
   
   ILightPtr my_sun;
   Vector<float> my_position;

   Tool my_tool;
   bool am_scrolling;

   // Variables for dragging track segments
   Point<int> drag_begin, drag_end;
   bool am_dragging;

   // GUI elements
   gui::ILayoutPtr layout;
   ISceneryPickerPtr building_picker, tree_picker;
   IRenderStatsPtr render_stats;
};

Editor::Editor(IMapPtr a_map) 
   : map(a_map), my_position(4.5f, -17.5f, -21.5f),
     my_tool(TRACK_TOOL), am_scrolling(false), am_dragging(false)
{
   my_sun = make_sun_light();
		
   build_gui();
		
   map->set_grid(true);
		
   log() << "Editing " << a_map->name();
}

Editor::~Editor()
{
   
}

void Editor::build_gui()
{
   using namespace placeholders;
    
   layout = gui::make_layout("layouts/editor.xml");
    
   layout->get("/tool_wnd/tools/track").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, TRACK_TOOL));
   layout->get("/tool_wnd/tools/raise").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, RAISE_TOOL));
   layout->get("/tool_wnd/tools/lower").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, LOWER_TOOL));
   layout->get("/tool_wnd/tools/level").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, LEVEL_TOOL));
   layout->get("/tool_wnd/tools/delete").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, DELETE_TOOL));
   layout->get("/tool_wnd/tools/start").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, START_TOOL));
   layout->get("/tool_wnd/tools/station").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, STATION_TOOL));
   layout->get("/tool_wnd/tools/building").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, BUILDING_TOOL));
   layout->get("/tool_wnd/tools/tree").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, TREE_TOOL));
   layout->get("/tool_wnd/tools/smooth").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::set_tool, this, SMOOTH_TOOL));
    
   layout->get("/lower/action_wnd/save").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::save, this));
    
   building_picker = make_building_picker(layout);
   tree_picker = make_tree_picker(layout);

   render_stats = make_render_stats(layout, "/fps/fps_label");
}

void Editor::set_map(IMapPtr a_map)
{
   map = a_map;
   map->set_grid(true);
}

void Editor::save()
{
   map->save();
}

// Calculate the bounds of the drag box accounting for the different
// possible directions of dragging
void Editor::drag_box_bounds(int& x_min, int& x_max, int &y_min, int& y_max) const
{
   x_min = min(drag_begin.x, drag_end.x);
   x_max = max(drag_begin.x, drag_end.x);

   y_min = min(drag_begin.y, drag_end.y);
   y_max = max(drag_begin.y, drag_end.y); 
}

// Render the next frame
void Editor::display(IGraphicsPtr a_context) const
{
   if (!map)
      return;
   
   a_context->set_camera(my_position, make_vector(45.0f, 45.0f, 0.0f));
 
   my_sun->apply();

   // Draw the highlight if we are dragging track
   if (am_dragging) {
      int xmin, xmax, ymin, ymax;
      drag_box_bounds(xmin, xmax, ymin, ymax);
      
      for (int x = xmin; x <= xmax; x++) {
	 for (int y = ymin; y <= ymax; y++)
	    map->highlight_tile(make_point(x, y), colour::WHITE);
      }         
   }
      
   map->render(a_context);
}

// Render the overlay
void Editor::overlay() const
{
   layout->render();
}

// Prepare the next frame
void Editor::update(IPickBufferPtr pick_buffer, int a_delta)
{
   render_stats->update(a_delta);
}

// True if the `a_first_point' is a valid track segment and it can
// connect to `a_second_point'
bool Editor::can_connect(const Point<int>& a_first_point,
   const Point<int>& a_second_point) const
{
   if (!map->is_valid_track(a_first_point))
      return false;

   ITrackSegmentPtr track = map->track_at(a_first_point);
   
   Vector<int> dir = make_vector(
      a_first_point.x - a_second_point.x,
      0,
      a_first_point.y - a_second_point.y).normalise();

   return track->is_valid_direction(dir)
      || track->is_valid_direction(-dir);
}

// Draw a single tile of straight track and check for collisions
// Returns `false' if track cannot be placed here
bool Editor::draw_track_tile(Point<int> where, track::Direction axis)
{
   // Ensure axis is only in the positive direction
   if (axis == -axis::X)
      axis = axis::X;
   else if (axis == -axis::Y)
      axis = axis::Y;
   
   if (map->is_valid_track(where)) {
      ITrackSegmentPtr merged = map->track_at(where)->merge_exit(where, axis);
      if (merged) {
	 map->set_track_at(where, merged);
	 return true;
      }
      else {
	 warn() << "Cannot merge track";
	 return false;
      }
   }
   else {
      bool level;
      Vector<float> slope = map->slope_at(where, axis, level);

      bool b_valid, a_valid;
      Vector<float> slope_before = map->slope_before(where, axis, b_valid);
      Vector<float> slope_after = map->slope_after(where, axis, a_valid);
               
      if (level) {
         const bool flat =
            abs(slope.y) < 0.001f
            && (!b_valid || abs(slope_before.y) < 0.001f)
            && (!a_valid || abs(slope_after.y) < 0.001);
         
         if (flat) {
            map->set_track_at(where, make_straight_track(axis));
            return true;
         }
         else {
            if (!b_valid || !a_valid) {
               warn() << "Cannot place track here";
               return false;
            }
            else {
               debug() << "slope=" << slope
                       << " before=" << slope_before
                       << " after=" << slope_after;

               map->set_track_at(where,
                  make_slope_track(axis, slope, slope_before, slope_after));

               return true;
            }
         }
      }
      else {
         warn() << "Track must be placed on level ground";
         return false;
      }
   }
}

// Special case where the user drags a rectangle of width 1
// This just draws straight track along the rectangle
void Editor::draw_dragged_straight(const track::Direction& an_axis, int a_length)
{
   Point<int> where = drag_begin;

   for (int i = 0; i < a_length; i++) {
      draw_track_tile(where, an_axis);
      
      where.x += an_axis.x;
      where.y += an_axis.z;
   }
}

// True if a track segment could be placed in its present location
bool Editor::can_place_track(ITrackSegmentPtr track)
{
   vector<Point<int> > covered;
   track->get_endpoints(covered);
   track->get_covers(covered);
   
   for (vector<Point<int> >::iterator it = covered.begin();
        it != covered.end(); ++it) {
      if (map->is_valid_track(*it)) {
         warn() << "Cannot place track here";
         return false;
      }
   }

   return true;
}

// Called when the user has finished dragging a rectangle for track
// Connect the beginning and end up in the simplest way possible
void Editor::draw_dragged_track()
{   
   track::Direction straight;  // Orientation for straight track section

   int xmin, xmax, ymin, ymax;
   drag_box_bounds(xmin, xmax, ymin, ymax);
   
   int xlen = abs(xmax - xmin) + 1;
   int ylen = abs(ymax - ymin) + 1;

   // Try to merge the start and end directly
   const track::Direction merge_axis =
      xlen > ylen ? (drag_begin.x < drag_end.x ? -axis::X : axis::X)
      : (drag_begin.y < drag_end.y ? -axis::Y : axis::Y);
   if (map->is_valid_track(drag_end)) {
      ITrackSegmentPtr merged =
	 map->track_at(drag_end)->merge_exit(drag_begin, merge_axis);

      if (merged) {
	 // Erase all the tiles covered
	 for (int x = xmin; x <= xmax; x++) {
	    for (int y = ymin; y <= ymax; y++)
	       map->erase_tile(x, y);
	 }
         
	 map->set_track_at(drag_end, merged);
	 return;
      }
   }
      
   // Normalise the coordinates so the start is always the one with
   // the smallest x-coordinate
   if (drag_begin.x > drag_end.x)
      swap(drag_begin, drag_end);

   track::Direction start_dir, end_dir;
   bool start_was_guess = false;
   bool end_was_guess = false;

   // Try to work out the direction of the track start
   if (can_connect(drag_begin.left(), drag_begin)
      || can_connect(drag_begin.right(), drag_begin)) {
      start_dir = axis::X;
   }
   else if (can_connect(drag_begin.up(), drag_begin)
      || can_connect(drag_begin.down(), drag_begin)) {
      start_dir = axis::Y;
   }
   else
      start_was_guess = true;

   // Try to work out the direction of the track end
   if (can_connect(drag_end.left(), drag_end)
      || can_connect(drag_end.right(), drag_end)) {
      end_dir = axis::X;
   }
   else if (can_connect(drag_end.up(), drag_end)
      || can_connect(drag_end.down(), drag_end)) {
      end_dir = axis::Y;
   }
   else
      end_was_guess = true;

   // If we have to guess both orientations use a heuristic to decide
   // between S-bends and curves
   if (end_was_guess && start_was_guess) {
      if (min(xlen, ylen) <= 2) {
	 if (xlen > ylen)
	    start_dir = end_dir = axis::X;
	 else
	    start_dir = end_dir = axis::Y;
      }
      else {
	 start_dir = axis::X;
	 end_dir = axis::Y;
      }
   }
   // Otherwise always prefer curves to S-bends
   else if (start_was_guess)
      start_dir = end_dir == axis::X ? axis::Y : axis::X;
   else if (end_was_guess)
      end_dir = start_dir == axis::X ? axis::Y : axis::X;
   
   if (xlen == 1 && ylen == 1) {
      // A single tile
      draw_track_tile(drag_begin, start_dir);
   }
   else if (xlen == 1)
      draw_dragged_straight(drag_begin.y < drag_end.y ? axis::Y : -axis::Y, ylen);
   else if (ylen == 1)
      draw_dragged_straight(axis::X, xlen);
   else if (start_dir == end_dir) {
      // An S-bend

      if (start_dir == axis::Y && drag_begin.y > drag_end.y)
         swap(drag_begin, drag_end);
      
      const int straight =
         (start_dir == axis::X
            ? drag_end.x - drag_begin.x
            : drag_end.y - drag_begin.y) + 1;
      const int offset =
         start_dir == axis::X
         ? drag_end.y - drag_begin.y
         : drag_end.x - drag_begin.x;
      ITrackSegmentPtr track = makeSBend(start_dir, straight, offset);
      const Point<int> where = drag_begin;
      track->set_origin(where.x, where.y, map->height_at(where));

      if (can_place_track(track))
	 map->set_track_at(where, track);
   }
   else {
      // Curves at the moment cannot be ellipses so lay track down
      // until the dragged area is a square
      while (xlen != ylen) {
	 if (xlen > ylen) {
	    // One of the ends must lie along the x-axis since all
	    // curves are through 90 degrees so extend that one
	    if (start_dir == axis::X) {
	       draw_track_tile(drag_begin, axis::X);
	       drag_begin.x++;
	    }
	    else {
	       draw_track_tile(drag_end, axis::X);
	       drag_end.x--;
	    }
	    xlen--;
	 }
	 else {
	    // Need to draw track along y-axis
	    if (start_dir == axis::Y) {
	       draw_track_tile(drag_begin, axis::Y);

	       // The y-coordinate for the drag points is not guaranteed
	       // to be sorted
	       if (drag_begin.y > drag_end.y)
		  drag_begin.y--;
	       else
		  drag_begin.y++;
	    }
	    else {
	       draw_track_tile(drag_end, axis::Y);
                                    
	       if (drag_begin.y > drag_end.y)
		  drag_end.y++;
	       else
		  drag_end.y--;
	    }
	    ylen--;
	 }
      }

      track::Angle start_angle, end_angle;
      Point<int> where;

      if (start_dir == axis::X && end_dir == axis::Y) {
	 if (drag_begin.y < drag_end.y) {
	    start_angle = 90;
	    end_angle = 180;
	    where = drag_end;
	 }
	 else {
	    start_angle = 0;
	    end_angle = 90;
	    where = drag_begin;
	 }
      }
      else {
	 if (drag_begin.y < drag_end.y) {
	    start_angle = 270;
	    end_angle = 360;
	    where = drag_begin;
	 }
	 else {
	    start_angle = 180;
	    end_angle = 270;
	    where = drag_end;
	 }
      }

      ITrackSegmentPtr track = make_curved_track(start_angle, end_angle, xlen);
      track->set_origin(where.x, where.y, map->height_at(where));

      if (can_place_track(track))
	 map->set_track_at(where, track);
   }
}

// Delete all objects in the area selected by the user
void Editor::delete_objects()
{
   int xmin, xmax, ymin, ymax;
   drag_box_bounds(xmin, xmax, ymin, ymax);

   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++)
	 map->erase_tile(x, y);
   }
}

// Plant trees at random locations in the dragged region
void Editor::plant_trees()
{
   int xmin, xmax, ymin, ymax;
   drag_box_bounds(xmin, xmax, ymin, ymax);

   const bool is_single_tile = (xmin == xmax) && (ymin == ymax);
   const float threshold = 0.9f;

   static Uniform<float> tree_rand(0.0f, 1.0f);
    
   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++) {
         const Point<int> p = make_point(x, y);
         
         if ((is_single_tile || tree_rand() > threshold) && map->empty_tile(p))
            map->add_scenery(p, tree_picker->get());
      }
   }
}

void Editor::on_mouse_move(IPickBufferPtr pick_buffer, int x, int y,
   int xrel, int yrel)
{
   if (am_dragging) {
      // Extend the selection rectangle
      map->set_pick_mode(true);
      IGraphicsPtr pick_context = pick_buffer->begin_pick(x, y);
      display(pick_context);
      int id = pick_buffer->end_pick();
      map->set_pick_mode(false);

      if (id > 0)
	 drag_end = map->pick_position(id);
   }
   else if (am_scrolling) {
      const float speed = 0.05f;
      
      my_position.x -= static_cast<float>(xrel) * speed;
      my_position.z -= static_cast<float>(xrel) * speed;
      
      my_position.x += static_cast<float>(yrel) * speed;
      my_position.z -= static_cast<float>(yrel) * speed;      
   }
}

void Editor::on_mouse_click(IPickBufferPtr pick_buffer, int x, int y,
   MouseButton a_button)
{   
   if (a_button == MOUSE_RIGHT) {
      // Start scrolling
      am_scrolling = true;
   }
   else if (a_button == MOUSE_LEFT) {
      bool clicked_onGUI = layout->click(x, y);

      if (!clicked_onGUI) {
	 // See if the user clicked on something in the map
	 map->set_pick_mode(true);
	 IGraphicsPtr pick_context = pick_buffer->begin_pick(x, y);
	 display(pick_context);
	 int id = pick_buffer->end_pick();
	 map->set_pick_mode(false);
         
	 if (id > 0) {
	    // Begin dragging a selection rectangle
	    Point<int> where = map->pick_position(id);
         
	    drag_begin = drag_end = where;
	    am_dragging = true;
	 }
      }
   }
   else if (a_button == MOUSE_WHEEL_UP) {
      my_position.y -= 0.5f;
   }
   else if (a_button == MOUSE_WHEEL_DOWN) {
      my_position.y += 0.5f;
   }
}

void Editor::on_mouse_release(IPickBufferPtr pick_buffer, int x, int y,
   MouseButton a_button)
{
   if (am_dragging) {
      // Stop dragging and perform the action
      switch (my_tool) {
      case TRACK_TOOL:
	 draw_dragged_track();
	 break;
      case RAISE_TOOL:
	 map->raise_area(drag_begin, drag_end);
	 break;
      case LOWER_TOOL:
	 map->lower_area(drag_begin, drag_end);
	 break;
      case LEVEL_TOOL:
	 map->level_area(drag_begin, drag_end);
	 break;
      case DELETE_TOOL:
	 delete_objects();
	 break;
      case START_TOOL:
	 map->set_start(drag_begin.x, drag_begin.y);
	 break;
      case STATION_TOOL:
	 map->extend_station(drag_begin, drag_end);
	 break;
      case BUILDING_TOOL:
         map->add_scenery(drag_begin, building_picker->get());
	 break;
      case TREE_TOOL:
	 plant_trees();
	 break;
      case SMOOTH_TOOL:
         map->smooth_area(drag_begin, drag_end);
         break;
      }
         
      am_dragging = false;
   }
   else if (am_scrolling) {
      am_scrolling = false;
   }
}

void Editor::on_key_up(SDLKey a_key)
{
   
}

void Editor::on_key_down(SDLKey a_key)
{
   switch (a_key) {
   case SDLK_g:
      // Toggle grid
      map->set_grid(true);
      break;
   case SDLK_PRINT:
      get_game_window()->take_screen_shot();
      break;
   default:
      break;
   }
}

// Create an instance of the editor screen
IScreenPtr make_editor_screen(IMapPtr a_map)
{
   return IScreenPtr(new Editor(a_map));
}
