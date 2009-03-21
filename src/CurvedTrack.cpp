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

#include <cmath>

#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;

// Concrete implementation of curved pieces of track
class CurvedTrack : public ITrackSegment {
public:
   CurvedTrack();
   ~CurvedTrack();

   void render() const;

   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength() const;

   Vector<double> offsetForDelta(double aDelta) const;
   Point<int> nextPosition() const;
private:
   int myX, myY;
   static GLUquadric* ourRailQuadric;
};

GLUquadric* CurvedTrack::ourRailQuadric(NULL);

CurvedTrack::CurvedTrack()
   : myX(0), myY(0)
{
   if (ourRailQuadric == NULL)
      ourRailQuadric = gluNewQuadric();
}

CurvedTrack::~CurvedTrack()
{

}

double CurvedTrack::segmentLength() const
{

}

Vector<double> CurvedTrack::offsetForDelta(double aDelta) const
{

}

Point<int> CurvedTrack::nextPosition() const
{

}

void CurvedTrack::render() const
{
   renderCurveRail();
}

ITrackSegmentPtr makeCurvedTrack()
{
   return ITrackSegmentPtr(new CurvedTrack);
}
