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

#include <stdexcept>
#include <cassert>

#include <GL/gl.h>
#include <GL/glu.h>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace boost;

// A section of track that allows travelling along both axis
class CrossoverTrack : public ITrackSegment,
                       public enable_shared_from_this<CrossoverTrack> {
public:
   CrossoverTrack() : myX(0), myY(0) {}
   ~CrossoverTrack() {}

   void setOrigin(int x, int y) { myX = x; myY = y; }
   
   void render() const;
   double segmentLength() const;
   TransformFunc transformFunc(const Track::Direction& aDirection) const;
   bool isValidDirection(const Track::Direction& aDirection) const;
   Track::Connection nextPosition(const Track::Direction& aDirection) const;
   void getEndpoints(std::list<Point<int> >& aList) const;
   ITrackSegmentPtr mergeExit(const Point<int>& aPoint,
                              const Track::Direction& aDirection);
   
private:
   void transform(const Track::Direction& aDirection, double aDelta) const;
   
   int myX, myY;
};

void CrossoverTrack::render() const
{
   // Render the y-going rails and sleepers
   glPushMatrix();

   renderStraightRail();
   
   glRotated(90.0, 0.0, 1.0, 0.0);
   glTranslated(-0.4, 0.0, 0.0);

   for (int i = 0; i < 4; i++) {
      renderSleeper();
      glTranslated(0.25, 0.0, 0.0);
   }
   
   glPopMatrix();

   // Render the x-going rails and sleepers
   glPushMatrix();
   
   glRotated(90.0, 0.0, 1.0, 0.0);

   renderStraightRail();
   
   glRotated(90.0, 0.0, 1.0, 0.0);
   glTranslated(-0.4, 0.0, 0.0);

   for (int i = 0; i < 4; i++) {
      renderSleeper();
      glTranslated(0.25, 0.0, 0.0);
   }

   glPopMatrix();
}

double CrossoverTrack::segmentLength() const
{
   return 1.0;
}

void CrossoverTrack::transform(const Track::Direction& aDirection,
                               double aDelta) const
{
   assert(aDelta < 1.0);
   
   bool backwards = aDirection == -Axis::X || aDirection == -Axis::Y;

   if (backwards) {
      aDelta = 1.0 - aDelta;
   }

   Track::Direction dir = backwards ? -aDirection : aDirection;

   const double xTrans = dir == Axis::X ? aDelta : 0;
   const double yTrans = dir == Axis::Y ? aDelta : 0;

   glTranslated(static_cast<double>(myX) + xTrans,
                0.0,
                static_cast<double>(myY) + yTrans);

   if (dir == Axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (backwards)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

ITrackSegment::TransformFunc
CrossoverTrack::transformFunc(const Track::Direction& aDirection) const
{
   if (!isValidDirection(aDirection))
      throw runtime_error
         ("Invalid direction on crossover: " + lexical_cast<string>(aDirection));

   return bind(&CrossoverTrack::transform, this, aDirection, _1);
}

bool CrossoverTrack::isValidDirection(const Track::Direction& aDirection) const
{
   return aDirection == Axis::X || aDirection == Axis::Y
      || aDirection == -Axis::Y || aDirection == -Axis::X;
}

Track::Connection
CrossoverTrack::nextPosition(const Track::Direction& aDirection) const
{
   
   if (aDirection == Axis::X)
      return make_pair(makePoint(myX + 1, myY), Axis::X);
   else if (aDirection == -Axis::X)
      return make_pair(makePoint(myX - 1, myY), -Axis::X);
   else if (aDirection == Axis::Y)
      return make_pair(makePoint(myX, myY + 1), Axis::Y);
   else if (aDirection == -Axis::Y)
      return make_pair(makePoint(myX, myY - 1), -Axis::Y);
   else
      throw runtime_error
         ("Invalid direction on crossover: " + lexical_cast<string>(aDirection));
}

void CrossoverTrack::getEndpoints(std::list<Point<int> >& aList) const
{
   aList.push_back(makePoint(myX, myY));
}

ITrackSegmentPtr CrossoverTrack::mergeExit(const Point<int>& aPoint,
                                           const Track::Direction& aDirection)
{
   if (aPoint == makePoint(myX, myY)
       && isValidDirection(aDirection))
      return shared_from_this();

   // No way to extend a crossover
   return ITrackSegmentPtr();
}

ITrackSegmentPtr makeCrossoverTrack()
{
   return ITrackSegmentPtr(new CrossoverTrack);
}
