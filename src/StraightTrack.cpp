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

// Concrete implementation of straight-line pieces of track
class StraightTrack : public ITrackSegment {
public:
   StraightTrack(Orientation anOrientation);
   ~StraightTrack();
   
   void render() const;
   
   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength() const { return 1.0; }

   Vector<double> offsetForDelta(double aDelta) const;
   Connection nextPosition(const Vector<int>& aDirection) const;
   bool isValidDirection(const Direction& aDirection) const;
   
   TransformFunc transformFunc() const;
private:
   void transform(double aDelta) const;
   
   int myX, myY;  // Absolute position
   Orientation myOrientation;
};

StraightTrack::StraightTrack(Orientation anOrientation)
   : myOrientation(anOrientation)
{
   
}

StraightTrack::~StraightTrack()
{
   debug() << "~StraightTrack";
}

void StraightTrack::transform(double aDelta) const
{
   assert(aDelta < 1.0);

   const double xTrans = myOrientation == ALONG_X ? aDelta : 0;
   const double yTrans = myOrientation == ALONG_Y ? aDelta : 0;

   glTranslated(static_cast<double>(myX) + xTrans,
                0.0,
                static_cast<double>(myY) + yTrans);

   if (myOrientation == ALONG_Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);
}

ITrackSegment::TransformFunc StraightTrack::transformFunc() const
{
   return bind(&StraightTrack::transform, this, _1);
}

bool StraightTrack::isValidDirection(const Direction& aDirection) const
{
   const Direction alongX = makeVector(1, 0, 0);
   const Direction alongY = makeVector(0, 0, 1);

   if (myOrientation == ALONG_X) {
      log() << "ALONG_X: " << aDirection << " == " << alongX;
      return aDirection == alongX || -aDirection == alongX;
   }
   else {
      log() << "ALONG_Y: " << aDirection << " == " << alongY;
      return aDirection == alongY || -aDirection == alongY;
   }
}

ITrackSegment::Connection
StraightTrack::nextPosition(const Vector<int>& aDirection) const
{
   const Direction alongX = makeVector(1, 0, 0);
   const Direction alongY = makeVector(0, 0, 1);

   const Direction trackDir =
      myOrientation == ALONG_X ? alongX : alongY;

   if (aDirection != trackDir && aDirection != -trackDir)
      throw runtime_error
         ("Invalid direction on straight track: "
          + lexical_cast<string>(aDirection)
          + " (should be parallel to "
          + lexical_cast<string>(trackDir) + ")");

   if (aDirection == alongX)
      return make_pair(makePoint(myX + 1, myY), alongX);
   else if (aDirection == -alongX)
      return make_pair(makePoint(myX - 1, myY), -alongX);
   else if (aDirection == alongY)
      return make_pair(makePoint(myX, myY + 1), alongY);
   else if (aDirection == -alongY)
      return make_pair(makePoint(myX, myY - 1), -alongY);
   else
      assert(false);
}

void StraightTrack::render() const
{
   glPushMatrix();

   if (myOrientation == ALONG_X)
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

ITrackSegmentPtr makeStraightTrack(Orientation anOrientation)
{
   return ITrackSegmentPtr(new StraightTrack(anOrientation));
}
