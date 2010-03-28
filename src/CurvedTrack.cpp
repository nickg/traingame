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
#include "XMLBuilder.hpp"

#include <cmath>
#include <cassert>
#include <stdexcept>
#include <set>

#include <GL/gl.h>
#include <boost/lexical_cast.hpp>
 
using namespace placeholders;
using namespace track;
using namespace boost;

// Concrete implementation of curved pieces of track
class CurvedTrack : public ITrackSegment,
                    public enable_shared_from_this<CurvedTrack> {
public:
   CurvedTrack(track::Angle aStartAngle, track::Angle aFinishAngle,
      int aRadius);
   ~CurvedTrack();

   void render() const;

   void setOrigin(int x, int y, float h);
   float segmentLength(const track::TravelToken& aToken) const;

   Connection nextPosition(const track::TravelToken& aToken) const;
   bool isValidDirection(const Direction& aDirection) const;
   void getEndpoints(vector<Point<int> >& aList) const;
   void getCovers(vector<Point<int> >& output) const;
   
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);

   xml::element toXml() const;
   track::TravelToken getTravelToken(track::Position aPosition,
      track::Direction aDirection) const;
    
   bool hasMultipleStates() const { return false; }
   void nextState() {}
   void prevState() {}
   void setStateRenderHint() {}
      
private:
   void transform(const track::TravelToken& aToken, float delta) const;
   Vector<int> cwEntryVector() const;
   Vector<int> ccwEntryVector() const;
   void ensureValidDirection(const Direction& aDirection) const;

   Point<int> origin;
   int baseRadius;
   track::Angle startAngle, finishAngle;
   float height;
};

CurvedTrack::CurvedTrack(track::Angle aStartAngle,
   track::Angle aFinishAngle,
   int aRadius)
   : origin(makePoint(0, 0)), baseRadius(aRadius),
     startAngle(aStartAngle), finishAngle(aFinishAngle),
     height(0.0f)
{
   
}

CurvedTrack::~CurvedTrack()
{

}

void CurvedTrack::setOrigin(int x, int y, float h)
{
   origin = makePoint(x, y);
   height = h;
}

track::TravelToken
CurvedTrack::getTravelToken(track::Position aPosition,
   track::Direction aDirection) const
{
   ensureValidDirection(aDirection);

   track::TravelToken tok = {
      aDirection,
      aPosition,
      bind(&CurvedTrack::transform, this, _1, _2),
      track::flatGradientFunc,
      1
   };
   return tok;
}

void CurvedTrack::transform(const track::TravelToken& aToken, float delta) const
{
   assert(delta < segmentLength(aToken));
   
   glTranslated(static_cast<double>(origin.x),
      height,
      static_cast<double>(origin.y));

   transformToOrigin(baseRadius, startAngle);

   bool backwards = aToken.direction == cwEntryVector();
   
   double ratio = delta / segmentLength(aToken);
   if (backwards)
      ratio = 1.0 - ratio;
      
   double angle = startAngle + (90.0 * ratio);

   glRotated(angle, 0.0, 1.0, 0.0);
   glTranslated(0.0, 0.0, static_cast<double>(baseRadius - 0.5));

   if (backwards)
      glRotatef(180.0, 0, 1, 0);
}

float CurvedTrack::segmentLength(const track::TravelToken& aToken) const
{
   // Assume curve is only through 90 degrees
   return M_PI * (static_cast<float>(baseRadius) - 0.5f) / 2.0f;
}

//
// Imagine the train is travelling in a circle like this:
//
// (0, 0)
//              180
//             [-1 0]
//        <-------------^
//        |             |
//        |             |
// [0 1]  |             | [0 -1] 90
// 270    |             |
//        |             |
//        V------------>|
//             [1 0]
//               0
//
// Above are the vectors for /counter/-clockwise movement
//

// The vector the train is moving on if it enters clockwise
Vector<int> CurvedTrack::cwEntryVector() const
{
   return makeVector<int>(-cos(degToRad(finishAngle)), 0,
      sin(degToRad(finishAngle)));
}

// The vector the train is moving on if it enters counter-clockwise
Vector<int> CurvedTrack::ccwEntryVector() const
{
   return makeVector<int>(cos(degToRad(startAngle)), 0.0,
      -sin(degToRad(startAngle)));
}

void CurvedTrack::ensureValidDirection(const Direction& aDirection) const
{
   if (!isValidDirection(aDirection))
      throw runtime_error
         ("Invalid direction on curved track from "
            + lexical_cast<string>(startAngle) + " to "
            + lexical_cast<string>(finishAngle) + " degrees: "
            + lexical_cast<string>(aDirection)
            + " (should be "
            + lexical_cast<string>(cwEntryVector()) + " or "
            + lexical_cast<string>(ccwEntryVector()) + ")");
}

bool CurvedTrack::isValidDirection(const Direction& aDirection) const
{
   return aDirection == cwEntryVector() || aDirection == ccwEntryVector();
}

Connection CurvedTrack::nextPosition(const track::TravelToken& aToken) const
{
   bool backwards;
   Vector<int> nextDir;
   if (aToken.direction == cwEntryVector()) {
      nextDir = -ccwEntryVector();
      backwards = true;
   }
   else if (aToken.direction == ccwEntryVector()) {
      nextDir = -cwEntryVector();
      backwards = false;
   }
   else
      assert(false);

   // Assuming 90 degree curves again
   const int cosEnd = static_cast<int>(cos(degToRad(finishAngle)));
   const int cosStart = static_cast<int>(cos(degToRad(startAngle)));
   const int sinEnd = static_cast<int>(sin(degToRad(finishAngle)));
   const int sinStart = static_cast<int>(sin(degToRad(startAngle)));

   int xDelta, yDelta;

   if (backwards)
      xDelta = yDelta = 0;
   else {
      xDelta = (baseRadius - 1) * (sinEnd - sinStart);
      yDelta = (baseRadius - 1) * (cosEnd - cosStart);
   }
   
   return make_pair(makePoint(origin.x + xDelta + nextDir.x,
         origin.y + yDelta + nextDir.z),
      nextDir);
}

void CurvedTrack::getEndpoints(vector<Point<int> >& aList) const
{
   aList.push_back(origin);

   // Assuming 90 degree curves again
   const int cosEnd = static_cast<int>(cos(degToRad(finishAngle)));
   const int cosStart = static_cast<int>(cos(degToRad(startAngle)));
   const int sinEnd = static_cast<int>(sin(degToRad(finishAngle)));
   const int sinStart = static_cast<int>(sin(degToRad(startAngle)));
   
   const int xDelta = (baseRadius - 1) * (sinEnd - sinStart);
   const int yDelta = (baseRadius - 1) * (cosEnd - cosStart);
   
   aList.push_back(makePoint(origin.x + xDelta, origin.y + yDelta));
}

void CurvedTrack::getCovers(vector<Point<int> >& output) const
{
   vector<Point<int> > exits;
   getEndpoints(exits);

   const Point<int>& start = exits.at(0);
   const Point<int>& finish = exits.at(1);
   
   Point<int> trueOrigin =
      (startAngle == 90 || startAngle == 270)
      ? makePoint(finish.x, start.y)
      : makePoint(start.x, finish.y);
   
   set<Point<int> > tmp;

   // A fiddle factor to put the cover tiles in the best location
   const float fiddleRadius = static_cast<float>(baseRadius) - 0.5f;

   const float sign = (startAngle == 0 || startAngle == 180) ? 1.0f : -1.0f;

   for (track::Angle angle = startAngle; angle < finishAngle; angle += 5) {
      float x = fiddleRadius * sign * cos(degToRad(angle));
      float y = fiddleRadius * sign * sin(degToRad(angle));
      Point<int> p = makePoint(static_cast<int>(x),
         static_cast<int>(y));

      if (abs(p.x) >= baseRadius || abs(p.y) >= baseRadius)
         continue;
      
      Point<int> actual = p + trueOrigin;

      if (actual != start && actual != finish)
         tmp.insert(actual);
   }

   copy(tmp.begin(), tmp.end(), back_inserter(output));
}

ITrackSegmentPtr CurvedTrack::mergeExit(Point<int> where, track::Direction dir)
{
   // See if this is already an exit
   if (isValidDirection(dir)) {
      vector<Point<int> > exits;
      getEndpoints(exits);

      for (vector<Point<int> >::iterator it = exits.begin();
           it != exits.end(); ++it)
         if (*it == where)
            return shared_from_this();
   }

   // No way to merge this as an exit
   return ITrackSegmentPtr();
}

void CurvedTrack::render() const
{
   glPushMatrix();
   
   glTranslatef(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   renderCurvedTrack(baseRadius, startAngle, finishAngle);

   glPopMatrix();
}

xml::element CurvedTrack::toXml() const
{
   return xml::element("curvedTrack")
      .addAttribute("startAngle", startAngle)
      .addAttribute("finishAngle", finishAngle)
      .addAttribute("radius", baseRadius);
}

ITrackSegmentPtr makeCurvedTrack(track::Angle aStartAngle,
   track::Angle aFinishAngle, int aRadius)
{
   assert(aStartAngle < aFinishAngle);
   
   return ITrackSegmentPtr
      (new CurvedTrack(aStartAngle, aFinishAngle, aRadius));
}
