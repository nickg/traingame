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

#include <tr1/memory>

// A map is a MxN array of floating point height values
// It also contains the track layout and any scenery items
class IMap {
public:
   virtual int width() const = 0;
   virtual int depth() const = 0;
   virtual double heightAt() const = 0;

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

   // The start location consists of both a position and
   // a direction vector
   virtual Track::Connection startLocation() const = 0;
   
   virtual void render(IGraphicsPtr aContext) const = 0;

   // Regenerate the display lists after updating the map
   virtual void rebuildDisplayLists() = 0;

   // Draw a white border around the given tile
   virtual void highlightTile(IGraphicsPtr aContext,
                              const Point<int>& aPoint) const = 0;
   
   // Given a pick name return the (x, y) co-ordinate
   virtual Point<int> pickPosition(unsigned aName) const = 0;

   // True if this names a valid tile
   virtual bool isValidTileName(unsigned aName) const = 0;
};

typedef std::tr1::shared_ptr<IMap> IMapPtr;

// Make an empty map
IMapPtr makeEmptyMap(int aWidth, int aHeight);

#endif
