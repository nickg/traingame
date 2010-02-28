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
#include "IXMLSerialisable.hpp"
#include "XMLBuilder.hpp"
#include "BezierCurve.hpp"
#include "IMesh.hpp"
#include "TrackCommon.hpp"
#include "OpenGLHelper.hpp"
#include "ILogger.hpp"

#include <cassert>
#include <stdexcept>

#include <boost/cast.hpp>

// Like StraightTrack but with a change of height
class SlopeTrack : public ITrackSegment {
public:
   SlopeTrack(track::Direction axis, Vector<float> slope,
      Vector<float> slopeBefore, Vector<float> slopeAfter);

   // ITrackSegment interface
   void render() const;
   void setOrigin(int x, int y, float h);
   double segmentLength(const track::TravelToken& token) const;
   track::TravelToken getTravelToken(track::Position pos,
      track::Direction dir) const;
   bool isValidDirection(const track::Direction& dir) const;
   track::Connection nextPosition(const track::TravelToken& token) const;
   void getEndpoints(vector<Point<int> >& output) const;
   void getCovers(vector<Point<int> >& output) const {};
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);
   
   bool hasMultipleStates() const { return false; }
   void nextState() {}
   void prevState() {}
   void setStateRenderHint() {}

   // IXMLSerialisable inteface
   xml::element toXml() const;

private:
   void ensureValidDirection(const track::Direction& dir) const;
   void transform(const track::TravelToken& token, double delta) const;
   
   Point<int> origin;
   float height;
   IMeshPtr railMesh;
   track::Direction axis;
   float length;
   bool flip;
};

SlopeTrack::SlopeTrack(track::Direction axis, Vector<float> slope,
   Vector<float> slopeBefore, Vector<float> slopeAfter)
   : height(0.0f), axis(axis)
{
   const float OFF = 0.1f;

   if ((flip = (slope.y < 0.0f))) {
      slope.y = abs(slope.y);
      slopeBefore.y = abs(slopeBefore.y);
      slopeAfter.y = abs(slopeAfter.y);

      swap(slopeBefore, slopeAfter);
   }

   const float hFactor0 = sqrt(OFF / (1 + slopeBefore.y * slopeBefore.y));
   const float hFactor1 = sqrt(OFF / (1 + slopeAfter.y * slopeAfter.y));

   const float xDelta0 = hFactor0;
   const float yDelta0 = hFactor0 * slopeBefore.y;

   const float xDelta1 = hFactor1;
   const float yDelta1 = hFactor1 * slopeAfter.y;
   
   Vector<float> p1 = makeVector(0.0f, 0.0f, 0.0f);
   Vector<float> p2 = makeVector(xDelta0, yDelta0, 0.0f);
   Vector<float> p3 = makeVector(1.0f - xDelta1, slope.y - yDelta1, 0.0f);
   Vector<float> p4 = makeVector(1.0f, slope.y, 0.0f);
      
   debug() << p1 << " " << p2 << " " << p3 << " " << p4;
   
   BezierCurve<float> curve = makeBezierCurve(p1, p2, p3, p4);
   length = curve.length;

   // TODO: we should cache these
   railMesh = makeBezierRailMesh(curve);
}

void SlopeTrack::render() const
{
   glPushMatrix();
   
   glTranslatef(0.0f, height, 0.0f);

   if (axis == axis::Y)
      glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);

   if (flip)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

   renderRailMesh(railMesh);
   
   glPopMatrix();
}

void SlopeTrack::setOrigin(int x, int y, float h)
{
   origin = makePoint(x, y);
   height = h;
}

double SlopeTrack::segmentLength(const track::TravelToken& token) const
{
   return length;
}

bool SlopeTrack::isValidDirection(const track::Direction& dir) const
{
   if (axis == axis::X)
      return dir == axis::X || -dir == axis::X;
   else
      return dir == axis::Y || -dir == axis::Y;
}

void SlopeTrack::ensureValidDirection(const track::Direction& dir) const
{
   if (!isValidDirection(dir))
      throw runtime_error
         ("Invalid direction on straight track: "
            + boost::lexical_cast<string>(dir)
            + " (should be parallel to "
            + boost::lexical_cast<string>(axis) + ")");
}

track::Connection SlopeTrack::nextPosition(
   const track::TravelToken& token) const
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

track::TravelToken SlopeTrack::getTravelToken(track::Position pos,
      track::Direction dir) const
{
   using namespace placeholders;
   
   ensureValidDirection(dir);

   track::TravelToken tok = {
      dir,
      pos,
      bind(&SlopeTrack::transform, this, _1, _2),
      1
   };
   return tok;
}

void SlopeTrack::transform(const track::TravelToken& token, double delta) const
{
   assert(delta < 1.0);

   if (token.direction == -axis)
      delta = 1.0 - delta;

   const double xTrans = axis == axis::X ? delta : 0;
   const double yTrans = axis == axis::Y ? delta : 0;

   glTranslated(static_cast<double>(origin.x) + xTrans,
      0.0,
      static_cast<double>(origin.y) + yTrans);

   if (axis == axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (token.direction == -axis)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

void SlopeTrack::getEndpoints(vector<Point<int> >& output) const
{
   output.push_back(origin);
}

ITrackSegmentPtr SlopeTrack::mergeExit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

xml::element SlopeTrack::toXml() const
{
   return xml::element("slopeTrack")
      .addAttribute("align", axis == axis::X ? "x" : "y");
}

ITrackSegmentPtr makeSlopeTrack(track::Direction axis, Vector<float> slope,
   Vector<float> slopeBefore, Vector<float> slopeAfter)
{
   return ITrackSegmentPtr(
      new SlopeTrack(axis, slope, slopeBefore, slopeAfter));
}
