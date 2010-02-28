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

#include "Platform.hpp"
#include "Maths.hpp"
#include "IXMLSerialisable.hpp"

#include <vector>
#include <set>

// Types used for specifying track segments
namespace track {
   // TODO: This only needs (x, y) position and should contain
   // separate layer field - but will do for now
   typedef Point<int> Position;
   typedef Vector<int> Direction;
   
   // Uniquely identifies the location of a train and its orientation
   // along a piece of track
   // Used for verifying whether bits of track can join together
   typedef pair<Position, Direction> Connection;

   // Angles for curved track
   typedef int Angle;

   struct TravelToken;
   typedef function<void (const TravelToken&, double)> TransformFunc;

   // Sums up all the information required to travel along a piece
   // of track
   struct TravelToken {
      // Direction of travel at /entry/
      Direction direction;

      // Position of entry
      Position position;

      // A function that transforms the location of the train so it will
      // render in the correct place for this track segment The functions
      // assumes that it is initially placed at the origin
      TransformFunc transformer;

      // Number of possible exits from this track segment given the direction
      // we are travelling in
      int numExits;

      // Wrapper for the above
      void transform(double aDelta) const
      {
         transformer(*this, aDelta);
      }
   };
}

// Orientations for straight track
namespace axis {
   const track::Direction X = makeVector(1, 0, 0);
   const track::Direction Y = makeVector(0, 0, 1);
}

struct ITrackSegment;
typedef shared_ptr<ITrackSegment> ITrackSegmentPtr;

// A segment of track which fits over a number of tiles
// Each track segment has an origin and one or more exits
struct ITrackSegment : IXMLSerialisable {   
   virtual ~ITrackSegment() {}

   // Render the track with the origin in the centre
   virtual void render() const = 0;

   // Set the absolute position of the track in the world
   virtual void setOrigin(int x, int y) = 0;

   // Get the length of this track segment
   virtual double segmentLength(const track::TravelToken& aToken) const = 0;

   // Get a travel token for this track segment starting at a particular
   // position and moving in a particular direciton
   virtual track::TravelToken getTravelToken(track::Position aPosition,
      track::Direction aDirection) const = 0;
                                      
   // True if a train can travel in this direction along the track
   virtual bool isValidDirection(const track::Direction& aDirection) const = 0;
   
   // Return the position of the next segment of track and the
   // orientation of the train.
   // Note that this may not actually be a valid track segment!
   // You should call IMap::isValidTrack before using it (this
   // will happen, e.g. if the train runs off the end of the line)
   // The token passed here must have been generated by this track
   // segment when the train entered it.
   virtual track::Connection nextPosition(const track::TravelToken& aToken)
      const = 0;

   // Add all the endpoints of the track segment to the given list
   // Note that an endpoint is not the same as what is returned
   // from `nextPosition' - e.g. a straight track that takes up
   // one tile has a single endpoint which is its origin
   virtual void getEndpoints(vector<Point<int> >& aList) const = 0;

   // Similar to endpoints, the `covers' of a track are the tiles
   // which are not endpoints but are underneath the track
   virtual void getCovers(vector<Point<int> >& output) const = 0;

   // Add an exit to this section of track possibly generating
   // a new track segment
   // If this is not possible a null pointer is returned
   // If it can be merged a new track segment is returned that
   // may be bigger than the origin segment
   // The track may already have an exit here in which case
   // a pointer to itself will be returned
   virtual ITrackSegmentPtr mergeExit(Point<int> where,
      track::Direction dir) = 0;

   // Some track segments may have several states - e.g. points
   // These functions change the track state
   virtual bool hasMultipleStates() const = 0;
   virtual void prevState() = 0;
   virtual void nextState() = 0;

   // Set a hint to display something about the track state on the next render
   // call - e.g display an arrow over points
   virtual void setStateRenderHint() = 0;
};

ITrackSegmentPtr makeStraightTrack(const track::Direction& aDirection);
ITrackSegmentPtr makeCurvedTrack(track::Angle aStartAngle,
   track::Angle aFinishAngle, int aRadius);
ITrackSegmentPtr makeCrossoverTrack();
ITrackSegmentPtr makePoints(track::Direction aDirection, bool reflect);
ITrackSegmentPtr makeSlope(track::Direction axis,
   Vector<float> slopeBefore, Vector<float> slopeAfter);

#endif
