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
   
   virtual Point<int> startLocation() const = 0;
   
   virtual void render(IGraphicsPtr aContext) const = 0;
};

typedef std::tr1::shared_ptr<IMap> IMapPtr;

// Make an empty map
IMapPtr makeEmptyMap(int aWidth, int aHeight);

#endif
