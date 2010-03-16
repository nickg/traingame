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
   float segmentLength(const track::TravelToken& token) const;
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
   void transform(const track::TravelToken& token, float delta) const;
   float gradient(const track::TravelToken& token, float delta) const;
   
   Point<int> origin;
   float height;
   IMeshPtr railMesh;
   track::Direction axis;
   float length, yOffset;
   BezierCurve<float> curve;
};

SlopeTrack::SlopeTrack(track::Direction axis, Vector<float> slope,
   Vector<float> slopeBefore, Vector<float> slopeAfter)
   : height(0.0f), axis(axis), yOffset(0.0f)
{
   const float OFF = 0.1f;

   assert(axis == axis::X || axis == axis::Y);
   
   const Vector<float> avgBefore = (slope + slopeBefore) / 2.0f;
   const Vector<float> avgAfter = (slope + slopeAfter) / 2.0f;

   if (slope.y < 0.0f)
      yOffset = abs(slope.y);

   const float hFactor0 = sqrt(OFF / (1 + avgBefore.y * avgBefore.y));
   const float hFactor1 = sqrt(OFF / (1 + avgAfter.y * avgAfter.y));

   const float xDelta0 = hFactor0;
   const float yDelta0 = hFactor0 * avgBefore.y;

   const float xDelta1 = hFactor1;
   const float yDelta1 = hFactor1 * avgAfter.y;
   
   Vector<float> p1 = makeVector(0.0f, 0.0f, 0.0f);
   Vector<float> p2 = makeVector(xDelta0, yDelta0, 0.0f);
   Vector<float> p3 = makeVector(1.0f - xDelta1, slope.y - yDelta1, 0.0f);
   Vector<float> p4 = makeVector(1.0f, slope.y, 0.0f);

   curve = makeBezierCurve(p1, p2, p3, p4);
   length = curve.length;

   railMesh = makeBezierRailMesh(curve);
}

void SlopeTrack::render() const
{
   glPushMatrix();
   
   glTranslatef(0.0f, height, 0.0f);

   if (axis == axis::Y)
      glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);

   renderRailMesh(railMesh);

   // Draw the sleepers
   glTranslatef(-0.5f, 0.0f, 0.0f);

   for (float t = 0.1f; t < 1.0f; t += 0.25f) {
      const Vector<float> curveValue = curve(t);

      const Vector<float> deriv = curve.deriv(t);
      const float angle =
         radToDeg<float>(atanf(deriv.y / deriv.x));

      glPushMatrix();
      
      glTranslatef(curveValue.x, curveValue.y, 0.0f);
      glRotatef(angle, 0.0f, 0.0f, 1.0f);
      
      renderSleeper();

      glPopMatrix();
   }
      
   glPopMatrix();
}

void SlopeTrack::setOrigin(int x, int y, float h)
{
   origin = makePoint(x, y);
   height = h + yOffset;
}

float SlopeTrack::segmentLength(const track::TravelToken& token) const
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
      bind(&SlopeTrack::gradient, this, _1, _2),
      1
   };
   return tok;
}

float SlopeTrack::gradient(const track::TravelToken& token, float delta) const
{
   assert(delta < length && delta >= 0.0f);
   
   if (token.direction == -axis)
      delta = length - delta;

   return curve.deriv(delta).y;
}

void SlopeTrack::transform(const track::TravelToken& token, float delta) const
{
   assert(delta < length && delta >= 0.0f);

#if 0
   debug() << "f(0)=" << curve(0.0f)
           << " f(0.5)=" << curve(0.5f)
           << " f(1.0)=" << curve(1.0f);

   debug() << "f'(0)=" << curve.deriv(0.0f)
           << " f'(0.5)=" << curve.deriv(0.5f)
           << " f'(1.0)=" << curve.deriv(1.0f);
#endif
   
   if (token.direction == -axis)
      delta = length - delta;

   const float curveDelta = delta / length;

   const Vector<float> curveValue = curve(curveDelta);
   
   const float xTrans = axis == axis::X ? curveValue.x : 0.0f;
   const float yTrans =curveValue.y;
   const float zTrans = axis == axis::Y ? curveValue.x : 0.0f;
   
   glTranslated(static_cast<double>(origin.x) + xTrans,
      height + yTrans,
      static_cast<double>(origin.y) + zTrans);

   if (axis == axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (token.direction == -axis)
      glRotated(-180.0, 0.0, 1.0, 0.0);

   const Vector<float> deriv = curve.deriv(curveDelta);
   const float angle =
      radToDeg<float>(atanf(deriv.y / deriv.x));

   if (token.direction == -axis)
      glRotatef(-angle, 0.0f, 0.0f, 1.0f);
   else
      glRotatef(angle, 0.0f, 0.0f, 1.0f);
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