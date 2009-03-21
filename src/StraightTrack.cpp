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

#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;

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
private:
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

Vector<double> StraightTrack::offsetForDelta(double aDelta) const
{
   assert(aDelta < 1.0);

   return makeVector(static_cast<double>(myX),
                     0.0,
                     static_cast<double>(myY) + aDelta);
}

Point<int> StraightTrack::nextPosition() const
{
   return makePoint(myX, myY + 1);
}

void StraightTrack::render() const
{
   renderStraightRail();
   
   // Left rail
   /*glPushMatrix();
   glTranslated(-0.3, 0.05, -0.5);
   renderStraightRail();
   /*glColor3d(0.7, 0.7, 0.7);
      gluCylinder(myRailQuadric,
               0.03, 0.03,  // Base, top
               1.0,         // Height
               15, 1);      // Slices, stacks
   glPopMatrix();

   // Right rail
   glPushMatrix();
   glTranslated(0.3, 0.05, -0.5);
   glColor3d(0.7, 0.7, 0.7);
   gluCylinder(myRailQuadric,
               0.03, 0.03,  // Base, top
               1.0,         // Height
               15, 1);      // Slices, stacks
               glPopMatrix();*/

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
