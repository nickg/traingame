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
//#include <tr1/bind>

#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;

// Concrete implementation of straight-line pieces of track
class StraightTrack : public ITrackSegment {
public:
   StraightTrack();
   ~StraightTrack();
   
   void render() const;
   
   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength() const { return 1.0; }

   Vector<double> offsetForDelta(double aDelta) const;
   Point<int> nextPosition() const;
   TransformFunc transformFunc() const;
private:
   void transform(double aDelta) const;
   
   int myX, myY;  // Absolute position
   GLUquadric* myRailQuadric;
};

StraightTrack::StraightTrack()
{
   myRailQuadric = gluNewQuadric();
}

StraightTrack::~StraightTrack()
{
   gluDeleteQuadric(myRailQuadric);
}

void StraightTrack::transform(double aDelta) const
{
   assert(aDelta < 1.0);
   
   glTranslated(static_cast<double>(myX),
                0.0,
                static_cast<double>(myY) + aDelta);
   glRotated(-90.0, 0.0, 1.0, 0.0);
}

ITrackSegment::TransformFunc StraightTrack::transformFunc() const
{
   return bind(&StraightTrack::transform, this, _1);
}

Point<int> StraightTrack::nextPosition() const
{
   return makePoint(myX, myY + 1);
}

void StraightTrack::render() const
{
   renderStraightRail();
   
   // Draw the sleepers
   glPushMatrix();
   glRotated(90.0, 0.0, 1.0, 0.0);
   glTranslated(-0.4, 0.0, 0.0);

   for (int i = 0; i < 4; i++) {
      renderSleeper();
      glTranslated(0.25, 0.0, 0.0);
   }
   
   glPopMatrix();
}

ITrackSegmentPtr makeStraightTrack()
{
   return ITrackSegmentPtr(new StraightTrack);
}
