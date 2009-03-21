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

// Concrete implementation of curved pieces of track
class CurvedTrack : public ITrackSegment {
public:
   CurvedTrack();
   ~CurvedTrack();

   void render() const;

   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength() const;

   Point<int> nextPosition() const;
   TransformFunc transformFunc() const;
private:
   void transform(double aDelta) const;
   
   int myX, myY, myBaseRadius;
};

CurvedTrack::CurvedTrack()
   : myX(0), myY(0), myBaseRadius(4)
{
   
}

CurvedTrack::~CurvedTrack()
{

}

void CurvedTrack::transform(double aDelta) const
{
   assert(aDelta < segmentLength());

   
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

Point<int> CurvedTrack::nextPosition() const
{
   return makePoint(1, 0);
}

void CurvedTrack::render() const
{
   renderCurveRail(4);
}

ITrackSegmentPtr makeCurvedTrack()
{
   return ITrackSegmentPtr(new CurvedTrack);
}
