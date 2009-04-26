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
#include <tr1/tuple>
#include <list>

namespace xml {
   struct element;
}

// Types used for specifying track segments
namespace Track {
   // TODO: This only needs (x, y) position and should contain
   // separate layer field - but will do for now
   typedef Point<int> Position;
   typedef Vector<int> Direction;
   
   // Uniquely identifies the location of a train and its orientation
   // along a piece of track
   // Used for verifying whether bits of track can join together
   typedef std::pair<Position, Direction> Connection;

   // Angles for curved track
   typedef int Angle;
}

// Orientations for straight track
namespace Axis {
   const Track::Direction X = makeVector(1, 0, 0);
   const Track::Direction Y = makeVector(0, 0, 1);
}

struct ITrackSegment;
typedef std::tr1::shared_ptr<ITrackSegment> ITrackSegmentPtr;

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
   // The functions assumes that it is initially placed at the origin
   virtual TransformFunc transformFunc(const Track::Direction& aDirection)
      const = 0;

   // True if a train can travel in this direction along the track
   virtual bool isValidDirection(const Track::Direction& aDirection) const = 0;
   
   // Return the position of the next segment of track and the
   // orientation of the train.
   // Note that this may not actually be a valid track segment!
   // You should call IMap::isValidTrack before using it (this
   // will happen, e.g. if the train runs off the end of the line)
   // The parameter `aDirection' specifies the direction of the
   // train when it /entered/ this segment. This must be valid
   // for this track segment - call isValidDirection first.
   virtual Track::Connection nextPosition(const Track::Direction& aDirection)
      const = 0;

   // Add all the endpoints of the track segment to the given list
   // Note that an endpoint is not the same as what is returned
   // from `nextPosition' - e.g. a straight track that takes up
   // one tile has a single endpoint which is its origin
   virtual void getEndpoints(std::list<Point<int> >& aList) const = 0;

   // Add an exit to this section of track possibly generating
   // a new track segment
   // If this is not possible a null pointer is returned
   // If it can be merged a new track segment is returned that
   // may be bigger than the origin segment
   // The track may already have an exit here in which case
   // a pointer to itself will be returned
   virtual ITrackSegmentPtr mergeExit(const Point<int>& aPoint,
                                      const Track::Direction& aDirection) = 0;

   // Convert the track segment to XML for writing out in the
   // map configuration file
   virtual xml::element toXml() const = 0;
};

ITrackSegmentPtr makeStraightTrack(const Track::Direction& aDirection);
ITrackSegmentPtr makeCurvedTrack(Track::Angle aStartAngle,
                                 Track::Angle aFinishAngle, int aRadius);
ITrackSegmentPtr makeCrossoverTrack();

#endif
