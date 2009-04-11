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

#include "ITrackSegment.hpp"
#include "TrackCommon.hpp"
#include "ILogger.hpp"

#include <cassert>
#include <stdexcept>

#include <GL/gl.h>
#include <GL/glu.h>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace boost;
using namespace Track;

// Concrete implementation of straight-line pieces of track
class StraightTrack : public ITrackSegment {
public:
   StraightTrack(const Direction& aDirection);
   ~StraightTrack();
   
   void render() const;
   
   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength() const { return 1.0; }

   Vector<double> offsetForDelta(double aDelta) const;
   Connection nextPosition(const Vector<int>& aDirection) const;
   bool isValidDirection(const Direction& aDirection) const;
   
   TransformFunc transformFunc(const Track::Direction& aDirection) const;
private:
   void transform(const Track::Direction& aDirection, double aDelta) const;
   void ensureValidDirection(const Track::Direction& aDirection) const;
   
   int myX, myY;  // Absolute position
   Direction myDirection;
};

StraightTrack::StraightTrack(const Direction& aDirection)
   : myDirection(aDirection)
{
   
}

StraightTrack::~StraightTrack()
{
   
}

void StraightTrack::transform(const Track::Direction& aDirection,
                              double aDelta) const
{
   assert(aDelta < 1.0);

   if (aDirection == -myDirection)
      aDelta = 1.0 - aDelta;

   const double xTrans = myDirection == Axis::X ? aDelta : 0;
   const double yTrans = myDirection == Axis::Y ? aDelta : 0;

   glTranslated(static_cast<double>(myX) + xTrans,
                0.0,
                static_cast<double>(myY) + yTrans);

   if (myDirection == Axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (aDirection == -myDirection)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

ITrackSegment::TransformFunc
StraightTrack::transformFunc(const Track::Direction& aDirection) const
{
   ensureValidDirection(aDirection);
   
   return bind(&StraightTrack::transform, this, aDirection, _1);
}

bool StraightTrack::isValidDirection(const Direction& aDirection) const
{
   if (myDirection == Axis::X)
      return aDirection == Axis::X || -aDirection == Axis::X;
   else
      return aDirection == Axis::Y || -aDirection == Axis::Y;
}

void StraightTrack::ensureValidDirection(const Direction& aDirection) const
{
   if (!isValidDirection(aDirection))
      throw runtime_error
         ("Invalid direction on straight track: "
          + lexical_cast<string>(aDirection)
          + " (should be parallel to "
          + lexical_cast<string>(myDirection) + ")");
}

Connection StraightTrack::nextPosition(const Direction& aDirection) const
{
   ensureValidDirection(aDirection);

   if (aDirection == Axis::X)
      return make_pair(makePoint(myX + 1, myY), Axis::X);
   else if (aDirection == -Axis::X)
      return make_pair(makePoint(myX - 1, myY), -Axis::X);
   else if (aDirection == Axis::Y)
      return make_pair(makePoint(myX, myY + 1), Axis::Y);
   else if (aDirection == -Axis::Y)
      return make_pair(makePoint(myX, myY - 1), -Axis::Y);
   else
      assert(false);
}

void StraightTrack::render() const
{
   glPushMatrix();

   if (myDirection == Axis::X)
      glRotated(90.0, 0.0, 1.0, 0.0);
   
   renderStraightRail();
   
   // Draw the sleepers
   glRotated(90.0, 0.0, 1.0, 0.0);
   glTranslated(-0.4, 0.0, 0.0);

   for (int i = 0; i < 4; i++) {
      renderSleeper();
      glTranslated(0.25, 0.0, 0.0);
   }
   
   glPopMatrix();
}

ITrackSegmentPtr makeStraightTrack(const Direction& aDirection)
{
   Direction realDir(aDirection);
   
   // Direction must either be along Axis::X or Axis::Y but we
   // allow the opositite direction here too
   if (realDir == -Axis::X || realDir == -Axis::Y)
      realDir = -realDir;

   if (realDir != Axis::X && realDir != Axis::Y)
      throw runtime_error("Illegal straight track direction: "
                          + lexical_cast<string>(aDirection));
   
   return ITrackSegmentPtr(new StraightTrack(realDir));
}
