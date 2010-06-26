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

#ifndef INC_IMAP_HPP
#define INC_IMAP_HPP

#include "IGraphics.hpp"
#include "ITrackSegment.hpp"
#include "IStation.hpp"
#include "IResource.hpp"
#include "IScenery.hpp"
#include "Colour.hpp"

#include <memory>
#include <string>

// A map is a MxN array of floating point height values
// It also contains the track layout and any scenery items
class IMap {
public:
   virtual int width() const = 0;
   virtual int depth() const = 0;

   // Return the track segment at the given position
   // It is invalid to call this with a position that doesn't
   // contain the *origin* of a track segment -- call is_valid_track
   // first
   virtual ITrackSegmentPtr track_at(const Point<int>& a_point) const = 0;

   // True if the given position is the origin of a track segment
   virtual bool is_valid_track(const Point<int>& a_point) const = 0;

   // Change the track segment at the given position
   // Set rebuild to true to update the display lists used to render
   // the map
   virtual void set_track_at(const Point<int>& a_point,
      ITrackSegmentPtr a_track) = 0;

   // Return the station at this track location or a null pointer
   virtual IStationPtr station_at(Point<int> a_point) const = 0;

   // Delete the contents of a tile
   virtual void erase_tile(int x, int y) = 0;

   // False if this tile has something in it (track, scenery, etc.)
   virtual bool empty_tile(Point<int> point) const = 0;

   // The start location consists of both a position and
   // a direction vector
   virtual track::Connection start() const = 0;
   
   virtual void render(IGraphicsPtr a_context) const = 0;

   // Draw a coloured highlight over the given tile
   virtual void highlight_tile(Point<int> point, Colour colour) const = 0;
   
   // Given a pick name return the (x, y) co-ordinate
   virtual Point<int> pick_position(unsigned a_name) const = 0;

   // True if this names a valid tile
   virtual bool is_valid_tileName(unsigned a_name) const = 0;

   // Save the map to its resource
   virtual void save() = 0;

   // Return the name of the map resource
   virtual string name() const = 0;

   // Change the start location
   // The second variant allows setting the direction as well
   virtual void set_start(int x, int y) = 0;
   virtual void set_start(int x, int y, int dirX, int dirY) = 0;

   // Toggle display of grid lines
   virtual void set_grid(bool on_off) = 0;

   // Toggle pick mode on and off
   // This turns of display of everything but the terrain
   // and things that can be clicked on
   virtual void set_pick_mode(bool on_off) = 0;

   // Make a hill or valley in the given area
   virtual void raise_area(const Point<int>& a_start_pos,
      const Point<int>& a_finish_pos) = 0;
   virtual void lower_area(const Point<int>& a_start_pos,
      const Point<int>& a_finish_pos) = 0;

   // Make all tiles in the area the same height
   virtual void level_area(Point<int> a_start_pos, Point<int> a_finish_pos) = 0;

   // Smooth the gradient along a strip
   virtual void smooth_area(Point<int> start, Point<int> finish) = 0;

   // Create a new station covering this area or extend an existing station
   virtual IStationPtr extend_station(Point<int> a_start_pos,
      Point<int> a_finish_pos) = 0;

   // Get the height above ground at a particular point
   virtual float height_at(float x, float y) const = 0;
   virtual float height_at(Point<int> where) const = 0;

   // Given a tile and an axis, return a vector indicating the slope
   // along that axis. `level' is set if the slope is the same across
   // the tile
   virtual Vector<float> slope_at(Point<int> where,
      track::Direction axis, bool& level) const = 0;

   // Similar to slope_at, but calculates slope of tile before and
   // after along `axis'
   virtual Vector<float> slope_before(Point<int> where,
      track::Direction axis, bool &valid) const = 0;
   virtual Vector<float> slope_after(Point<int> where,
      track::Direction axis, bool &valid) const = 0;

   // Place a tree, building, etc. at a location
   virtual void add_scenery(Point<int> where, ISceneryPtr s) = 0;
   
};

typedef shared_ptr<IMap> IMapPtr;

// Make an empty map inside a resource
IMapPtr make_empty_map(const string& a_res_id, int a_width, int a_height);

// Load a map from a resource
IMapPtr load_map(const string& a_res_id);

#endif
