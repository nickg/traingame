//
//  Copyright (C) 2010  Nick Gasson
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
#include "BezierCurve.hpp"
#include "OpenGLHelper.hpp"

#include <cassert>
#include <boost/lexical_cast.hpp>

// Spline curves which start and finish in the same direction
class SBend : public ITrackSegment {
public:
   SBend(track::Direction dir, int xoff, int yoff);

   // ITrackSegment interface
   void render() const;
   void setOrigin(int x, int y, float h);
   float segmentLength(const track::TravelToken& token) const;
   bool isValidDirection(const track::Direction& dir) const;
   track::Connection nextPosition(const track::TravelToken& token) const;
   void getEndpoints(vector<Point<int> >& output) const;
   void getCovers(vector<Point<int> >& output) const;
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);
   track::TravelToken getTravelToken(track::Position pos,
      track::Direction dir) const;
   void nextState() {}
   void prevState() {}
   bool hasMultipleStates() const { return false; }
   void setStateRenderHint() {}

   // IXMLSerialisable interface
   xml::element toXml() const;

private:
   void transform(const track::TravelToken& token, float delta) const;
   void ensureValidDirection(track::Direction dir) const;

   Point<int> origin;
   int xOffset, yOffset;
   float height;
   track::Direction axis;

   BezierCurve<float> curve;
   IMeshPtr railMesh;
};

SBend::SBend(track::Direction dir, int xoff, int yoff)
   : xOffset(xoff), yOffset(yoff),
     height(0.0f),
     axis(dir)
{
   assert(xoff > 0);
   assert(yoff > 0);
   
   debug() << "SBend axis=" << axis << " xoff=" << xoff << " yoff=" << yoff;

   static const float PINCH = 1.0f;
   const float pinchX = dir == axis::X ? PINCH : 0.0f;
   const float pinchY = dir == axis::Y ? PINCH : 0.0f;

   const float xoffF = static_cast<float>(xoff - (dir == axis::Y ? 1 : 0));
   const float yoffF = static_cast<float>(yoff - (dir == axis::X ? 1 : 0));
   
   Vector<float> p1 = makeVector(0.0f, 0.0f, 0.0f);
   Vector<float> p2 = makeVector(pinchX, 0.0f, pinchY);
   Vector<float> p3 = makeVector(xoffF - pinchX, 0.0f, yoffF - pinchY);
   Vector<float> p4 = makeVector(xoffF, 0.0f, yoffF);

   curve = makeBezierCurve(p1, p2, p3, p4);
   railMesh = makeBezierRailMesh(curve);

   debug() << "f(0) = " << curve(0.0f)
           << " f(1) = " << curve(1.0f);
}

void SBend::setOrigin(int x, int y, float h)
{
   origin = makePoint(x, y);
   height = h;
}
   
void SBend::render() const
{
   glPushMatrix();

   glTranslatef(0.0f, height, 0.0f);

   renderRailMesh(railMesh);
   
   glPopMatrix();
}

float SBend::segmentLength(const track::TravelToken& token) const
{
   return curve.length;
}

bool SBend::isValidDirection(const track::Direction& dir) const
{
   if (axis == axis::X)
      return dir == axis::X || -dir == axis::X;
   else
      return dir == axis::Y || -dir == axis::Y;
}

track::Connection SBend::nextPosition(const track::TravelToken& token) const
{
   ensureValidDirection(token.direction);

   if (token.direction == axis::X)
      return make_pair(makePoint(origin.x + 1, origin.y), axis::X);
   else if (token.direction == -axis::X)
      return make_pair(makePoint(origin.x - 1, origin.y), -axis::X);
   else if (token.direction == axis::Y)
      return make_pair(makePoint(origin.x, origin.y + 1), axis::Y);
   else if (token.direction == -axis::Y)
      return make_pair(makePoint(origin.x, origin.y - 1), -axis::Y);
   else
      assert(false);
}

void SBend::getEndpoints(vector<Point<int> >& output) const
{
   output.push_back(origin);
   output.push_back(origin + makePoint(xOffset, yOffset));
}

void SBend::getCovers(vector<Point<int> >& output) const
{
   // TODO
}

ITrackSegmentPtr SBend::mergeExit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

track::TravelToken SBend::getTravelToken(track::Position pos,
   track::Direction dir) const
{
   assert(false);
}

void SBend::transform(const track::TravelToken& token, float delta) const
{
   assert(false);
}

void SBend::ensureValidDirection(track::Direction dir) const
{
   if (!isValidDirection(dir))
      throw runtime_error
         ("Invalid direction on straight track: "
            + boost::lexical_cast<string>(dir)
            + " (should be parallel to "
            + boost::lexical_cast<string>(axis) + ")");
}

xml::element SBend::toXml() const
{
   assert(false);
}

ITrackSegmentPtr makeSBend(track::Direction dir, int xoff, int yoff)
{
   return ITrackSegmentPtr(new SBend(dir, xoff, yoff));
}
