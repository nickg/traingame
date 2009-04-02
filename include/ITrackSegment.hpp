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

#ifndef INC_ITRACKSEGMENT_HPP
#define INC_ITRACKSEGMENT_HPP

#include "Maths.hpp"

#include <tr1/memory>
#include <tr1/functional>

// A segment of track which fits over a number of tiles
// Each track segment has an origin and one or more exits
struct ITrackSegment {
   virtual ~ITrackSegment() {}

   typedef std::tr1::function<void (double)> TransformFunc;
   
   // Render the track with the origin in the centre
   virtual void render() const = 0;

   // Set the absolute position of the track in the world
   virtual void setOrigin(int x, int y) = 0;

   // Get the length of this track segment
   virtual double segmentLength() const = 0;

   // Return a function that transforms the location of the train
   // so it will render in the correct place for this track segment
   virtual TransformFunc transformFunc() const = 0;

   // Return the position of the next segment of track
   // Note that this may not actually be a valid track segment!
   // You should call IMap::isValidTrack before using it (this
   // will happen, e.g. if the train runs off the end of the line)
   virtual Point<int> nextPosition() const = 0;
};

typedef std::tr1::shared_ptr<ITrackSegment> ITrackSegmentPtr;

// Orientations for straight track
enum Orientation {
   ALONG_X, ALONG_Y
};

ITrackSegmentPtr makeStraightTrack(Orientation anOrientation);
ITrackSegmentPtr makeCurvedTrack();

#endif
