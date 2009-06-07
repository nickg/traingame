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
#include "XMLBuilder.hpp"
#include "ILogger.hpp"

#include <GL/gl.h>

// Forks in the track
class Points : public ITrackSegment {
public:
   Points();

   // ITrackSegment interface
   void render() const;
   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength() const;
   TransformFunc transformFunc(const track::Direction& aDirection) const;
   bool isValidDirection(const track::Direction& aDirection) const;
   track::Connection nextPosition(const track::Direction& aDirection) const;
   void getEndpoints(std::list<Point<int> >& aList) const;
   ITrackSegmentPtr mergeExit(const Point<int>& aPoint,
                              const track::Direction& aDirection);
   xml::element toXml() const;
private:
   int myX, myY;
};

Points::Points()
   : myX(0), myY(0)
{
   
}

void Points::render() const
{   
   glPushMatrix();

   renderHypTanRail();

   glPushMatrix();
   glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

   for (int i = 0; i < 3; i++) {
      renderStraightRail();
      glTranslatef(0.0f, 0.0f, 1.0f);
   }
   
   glPopMatrix();

   // Draw the sleepers
   glTranslatef(-0.4f, 0.0f, 0.0f);
   
   for (int i = 0; i < 12; i++) {
      renderSleeper();
      glTranslatef(0.25f, 0.0f, 0.0f);
   }
   
   glPopMatrix();
}

double Points::segmentLength() const
{
   return 1.0;
}

ITrackSegment::TransformFunc
Points::transformFunc(const track::Direction& aDirection) const
{
   
}

bool Points::isValidDirection(const track::Direction& aDirection) const
{
   return true;
}

track::Connection Points::nextPosition(const track::Direction& aDirection) const
{

}

void Points::getEndpoints(std::list<Point<int> >& aList) const
{
   aList.push_back(makePoint(myX, myY));
}

ITrackSegmentPtr Points::mergeExit(const Point<int>& aPoint,
                                   const track::Direction& aDirection)
{

}

xml::element Points::toXml() const
{

}

ITrackSegmentPtr makePoints()
{
   return ITrackSegmentPtr(new Points);
}
