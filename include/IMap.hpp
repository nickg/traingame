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
   // contain the *origin* of a track segment -- call isValidTrack
   // first
   virtual ITrackSegmentPtr trackAt(const Point<int>& aPoint) const = 0;

   // True if the given position is the origin of a track segment
   virtual bool isValidTrack(const Point<int>& aPoint) const = 0;

   // Change the track segment at the given position
   // Set rebuild to true to update the display lists used to render
   // the map
   virtual void setTrackAt(const Point<int>& aPoint,
      ITrackSegmentPtr aTrack) = 0;

   // Return the station at this track location or a null pointer
   virtual IStationPtr stationAt(Point<int> aPoint) const = 0;

   // Delete the contents of a tile
   virtual void eraseTile(int x, int y) = 0;

   // The start location consists of both a position and
   // a direction vector
   virtual track::Connection start() const = 0;
   
   virtual void render(IGraphicsPtr aContext) const = 0;

   // Draw a coloured highlight over the given tile
   virtual void highlightTile(Point<int> point, Colour colour) const = 0;
   
   // Given a pick name return the (x, y) co-ordinate
   virtual Point<int> pickPosition(unsigned aName) const = 0;

   // True if this names a valid tile
   virtual bool isValidTileName(unsigned aName) const = 0;

   // Save the map to its resource
   virtual void save() = 0;

   // Return the name of the map resource
   virtual string name() const = 0;

   // Change the start location
   // The second variant allows setting the direction as well
   virtual void setStart(int x, int y) = 0;
   virtual void setStart(int x, int y, int dirX, int dirY) = 0;

   // Toggle display of grid lines
   virtual void setGrid(bool onOff) = 0;

   // Toggle pick mode on and off
   // This turns of display of everything but the terrain
   // and things that can be clicked on
   virtual void setPickMode(bool onOff) = 0;

   // Make a hill or valley in the given area
   virtual void raiseArea(const Point<int>& aStartPos,
      const Point<int>& aFinishPos) = 0;
   virtual void lowerArea(const Point<int>& aStartPos,
      const Point<int>& aFinishPos) = 0;

   // Make all tiles in the area the same height
   virtual void levelArea(Point<int> aStartPos, Point<int> aFinishPos) = 0;

   // Create a new station covering this area or extend an existing station
   virtual IStationPtr extendStation(Point<int> aStartPos,
      Point<int> aFinishPos) = 0;

   // Place a building at this location
   virtual void placeBuilding(Point<int> point, ISceneryPtr building) = 0;

   // Get the height above ground at a particular point
   virtual float heightAt(float x, float y) const = 0;

   // Place a tree, etc. at a location
   virtual void addScenery(Point<int> where, ISceneryPtr s) = 0;
   
};

typedef shared_ptr<IMap> IMapPtr;

// Make an empty map inside a resource
IMapPtr makeEmptyMap(const string& aResId, int aWidth, int aHeight);

// Load a map from a resource
IMapPtr loadMap(const string& aResId);

#endif
