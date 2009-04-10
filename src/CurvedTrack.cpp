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
#include <cassert>

#include <GL/gl.h>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace Track;

// Concrete implementation of curved pieces of track
class CurvedTrack : public ITrackSegment {
public:
   CurvedTrack(Track::Angle aStartAngle, Track::Angle aFinishAngle,
               int aRadius);
   ~CurvedTrack();

   void render() const;

   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength() const;

   Connection nextPosition(const Vector<int>& aDirection) const;
   TransformFunc transformFunc() const;
   bool isValidDirection(const Direction& aDirection) const;
private:
   void transform(double aDelta) const;
   
   int myX, myY, myBaseRadius;
   double myStartAngle, myFinishAngle;
};

CurvedTrack::CurvedTrack(Track::Angle aStartAngle,
                         Track::Angle aFinishAngle,
                         int aRadius)
   : myX(0), myY(0), myBaseRadius(aRadius)
{
   myStartAngle = (static_cast<double>(aStartAngle)*M_PI)/180.0;
   myFinishAngle = (static_cast<double>(aFinishAngle)*M_PI)/180.0;
}

CurvedTrack::~CurvedTrack()
{

}

void CurvedTrack::transform(double aDelta) const
{
   assert(aDelta < segmentLength());

   glTranslated(static_cast<double>(myX + myBaseRadius - 1) + 0.5,
                0.0,
                static_cast<double>(myY) - 0.5);

   glBegin(GL_LINES);
   glVertex3d(0.0, -5.0, 0.0);
   glVertex3d(0.0, 5.0, 0.0);
   glEnd();

   double ratio = aDelta / segmentLength();

   double angle = 90.0 * ratio;

   glRotated(-90.0 + angle, 0.0, 1.0, 0.0);
   glTranslated(0.5, 0.0, static_cast<double>(myBaseRadius) - 0.5);
   
   glBegin(GL_LINES);
   glVertex3d(0.0, -5.0, 0.0);
   glVertex3d(0.0, 5.0, 0.0);
   glEnd();
}

double CurvedTrack::segmentLength() const
{
   // Assume curve is only through 90 degrees
   return M_PI * (static_cast<double>(myBaseRadius) - 0.5) / 2.0;
}

ITrackSegment::TransformFunc CurvedTrack::transformFunc() const
{
   return bind(&CurvedTrack::transform, this, _1);
}

bool CurvedTrack::isValidDirection(const Direction& aDirection) const
{
   return true; // TODO!
}

Connection CurvedTrack::nextPosition(const Vector<int>& aDirection) const
{
   return make_pair(makePoint(myX + myBaseRadius, myY + myBaseRadius - 1),
                    makeVector(1, 0, 0));                    
}

void CurvedTrack::render() const
{
   renderCurvedTrack(myBaseRadius, myStartAngle, myFinishAngle);
}

ITrackSegmentPtr makeCurvedTrack(Track::Angle aStartAngle,
                                 Track::Angle aFinishAngle, int aRadius)
{
   assert(aStartAngle < aFinishAngle);
   
   return ITrackSegmentPtr
      (new CurvedTrack(aStartAngle, aFinishAngle, aRadius));
}
