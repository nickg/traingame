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

#include "ITrackSegment.hpp"
#include "TrackCommon.hpp"
#include "ILogger.hpp"
#include "XMLBuilder.hpp"
#include "Matrix.hpp"
#include "OpenGLHelper.hpp"

#include <cassert>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

using namespace placeholders;
using namespace boost;
using namespace track;

// Concrete implementation of straight-line pieces of track
class StraightTrack : public ITrackSegment,
                      public enable_shared_from_this<StraightTrack>,
                      private StraightTrackHelper,
                      private SleeperHelper {
public:
   StraightTrack(const Direction& aDirection);
   ~StraightTrack();
   
   void render() const {}
   void merge(IMeshBufferPtr buf) const;
   
   void setOrigin(int x, int y, float h);
   float segmentLength(const track::TravelToken& token) const { return 1.0f; }

   Connection nextPosition(const track::TravelToken& aDirection) const;
   bool isValidDirection(const Direction& aDirection) const;
   void getEndpoints(vector<Point<int> >& aList) const;
   void getCovers(vector<Point<int> >& output) const { }
   
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);
   track::TravelToken getTravelToken(track::Position aPosition,
      track::Direction aDirection) const;

   bool hasMultipleStates() const { return false; }
   void nextState() {}
   void prevState() {}
   void setStateRenderHint() {}

   // IXMLSerialisable interface
   xml::element toXml() const;
   
private:
   void transform(const track::TravelToken& aToken, float delta) const;
   void ensureValidDirection(const track::Direction& aDirection) const;
   
   Point<int> origin;  // Absolute position
   Direction direction;
   float height;
};

StraightTrack::StraightTrack(const Direction& aDirection)
   : direction(aDirection), height(0.0f)
{
   
}

StraightTrack::~StraightTrack()
{
   
}

void StraightTrack::setOrigin(int x, int y, float h)
{
   origin = makePoint(x, y);
   height = h;
}

track::TravelToken
StraightTrack::getTravelToken(track::Position aPosition,
   track::Direction aDirection) const
{
   ensureValidDirection(aDirection);

   track::TravelToken tok = {
      aDirection,
      aPosition,
      bind(&StraightTrack::transform, this, _1, _2),
      track::flatGradientFunc,
      1
   };
   return tok;
}

void StraightTrack::transform(const track::TravelToken& aToken,
   float delta) const
{
   assert(delta < 1.0);

   if (aToken.direction == -direction)
      delta = 1.0 - delta;

   const float xTrans = direction == axis::X ? delta : 0;
   const float yTrans = direction == axis::Y ? delta : 0;

   glTranslated(static_cast<double>(origin.x) + xTrans,
      height,
      static_cast<double>(origin.y) + yTrans);

   if (direction == axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (aToken.direction == -direction)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

ITrackSegmentPtr StraightTrack::mergeExit(Point<int> where,
   track::Direction dir)
{
#if 1
   debug() << "mergeExit where=" << where
           << " dir=" << dir
           << " me=" << origin
           << " mydir=" << direction;
#endif

   // See if we can make this a crossover track
   if (direction != dir && where == origin)
      return makeCrossoverTrack();

   // See if we can make some points
   if (isValidDirection(dir)) {
      // X-aligned points
      if (where == origin + makePoint(-2, 1))
         return makePoints(-axis::X, true);
      else if (where == origin + makePoint(-2, -1))
         return makePoints(-axis::X, false);
      else if (where == origin + makePoint(2, 1))
         return makePoints(axis::X, false);
      else if (where == origin + makePoint(2, -1))
         return makePoints(axis::X, true);

      // Y-aligned points
      if (where == origin + makePoint(1, -2))
         return makePoints(-axis::Y, false);
      else if (where == origin + makePoint(-1, -2))
         return makePoints(-axis::Y, true);
      else if (where == origin + makePoint(1, 2))
         return makePoints(axis::Y, true);
      else if (where == origin + makePoint(-1, 2))
         return makePoints(axis::Y, false);
   }
   
   // Not possible to merge
   return ITrackSegmentPtr();
}

bool StraightTrack::isValidDirection(const Direction& aDirection) const
{
   if (direction == axis::X)
      return aDirection == axis::X || -aDirection == axis::X;
   else
      return aDirection == axis::Y || -aDirection == axis::Y;
}

void StraightTrack::getEndpoints(vector<Point<int> >& aList) const
{
   aList.push_back(origin);
}

void StraightTrack::ensureValidDirection(const Direction& aDirection) const
{
   if (!isValidDirection(aDirection))
      throw runtime_error
         ("Invalid direction on straight track: "
            + lexical_cast<string>(aDirection)
            + " (should be parallel to "
            + lexical_cast<string>(direction) + ")");
}

Connection StraightTrack::nextPosition(const track::TravelToken& aToken) const
{
   ensureValidDirection(aToken.direction);

   if (aToken.direction == axis::X)
      return make_pair(makePoint(origin.x + 1, origin.y), axis::X);
   else if (aToken.direction == -axis::X)
      return make_pair(makePoint(origin.x - 1, origin.y), -axis::X);
   else if (aToken.direction == axis::Y)
      return make_pair(makePoint(origin.x, origin.y + 1), axis::Y);
   else if (aToken.direction == -axis::Y)
      return make_pair(makePoint(origin.x, origin.y - 1), -axis::Y);
   else
      assert(false);
}

void StraightTrack::merge(IMeshBufferPtr buf) const
{
   Vector<float> off = makeVector(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   float yAngle = direction == axis::X ? 90.0f : 0.0f;

   mergeStraightRail(buf, off, yAngle);

   yAngle += 90.0f;

   off += rotate(makeVector(-0.4f, 0.0f, 0.0f), yAngle, 0.0f, 1.0f, 0.0f);
   
   for (int i = 0; i < 4; i++) {
      mergeSleeper(buf, off, yAngle);

      off += rotate(makeVector(0.25f, 0.0f, 0.0f), yAngle, 0.0f, 1.0f, 0.0f);
   }  
}

xml::element StraightTrack::toXml() const
{
   return xml::element("straightTrack")
      .addAttribute("align", direction == axis::X ? "x" : "y");
}

ITrackSegmentPtr makeStraightTrack(const Direction& aDirection)
{
   Direction realDir(aDirection);
   
   // Direction must either be along axis::X or axis::Y but we
   // allow the opositite direction here too
   if (realDir == -axis::X || realDir == -axis::Y)
      realDir = -realDir;

   if (realDir != axis::X && realDir != axis::Y)
      throw runtime_error("Illegal straight track direction: "
         + lexical_cast<string>(aDirection));
   
   return ITrackSegmentPtr(new StraightTrack(realDir));
}
