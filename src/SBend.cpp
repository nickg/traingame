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
#include <map>

#include <boost/lexical_cast.hpp>

// Spline curves which start and finish in the same direction
class SBend : public ITrackSegment {
public:
   SBend(track::Direction dir, int straight, int off);

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
   const int straight, offset;
   float height;
   track::Direction axis;

   BezierCurve<float> curve;
   IMeshPtr railMesh;

   typedef tuple<int, int> Parameters;
   typedef map<Parameters, IMeshPtr> MeshCache; 
   static MeshCache meshCache;
};

SBend::MeshCache SBend::meshCache;

SBend::SBend(track::Direction dir, int straight, int off)
   : straight(straight), offset(off),
     height(0.0f),
     axis(dir)
{
   assert(straight > 0);

   const float pinch = static_cast<float>(straight) / 3.0f;

   const int reflect = (axis == axis::Y ? -1 : 1);
   
   const float offsetF = static_cast<float>(offset * reflect);
   const float straightF = static_cast<float>(straight);
   
   Vector<float> p1 = makeVector(0.0f, 0.0f, 0.0f);
   Vector<float> p2 = makeVector(pinch, 0.0f, 0.0f);
   Vector<float> p3 = makeVector(straightF - pinch, 0.0f, offsetF);
   Vector<float> p4 = makeVector(straightF, 0.0f, offsetF);

   curve = makeBezierCurve(p1, p2, p3, p4);

   Parameters parms = make_tuple(straight, offset * reflect);
   MeshCache::iterator it = meshCache.find(parms);
   if (it == meshCache.end()) {
      railMesh = makeBezierRailMesh(curve);
      meshCache[parms] = railMesh;
   }
   else
      railMesh = (*it).second;
}

void SBend::setOrigin(int x, int y, float h)
{
   origin = makePoint(x, y);
   height = h;
}
   
void SBend::render() const
{
   glPushMatrix();

   glTranslatef(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   if (axis == axis::Y)
      glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);

   glPushMatrix();
   
   glTranslatef(-0.5f, 0.0f, 0.0f);
   railMesh->render();
   
   glPopMatrix();
   
   for (float i = 0.2f; i < curve.length; i += 0.25f) {
      glPushMatrix();
      
      Vector<float> v = curve(i / curve.length);

      glTranslatef(v.x - 0.5f, 0.0f, v.z);
      
      const Vector<float> deriv = curve.deriv(i / curve.length);
      const float angle =
         radToDeg<float>(atanf(deriv.z / deriv.x));

      glRotatef(-angle, 0.0f, 1.0f, 0.0f);

      renderSleeper();
      
      glPopMatrix();
   }
   
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

   Point<int> disp;
   
   if (token.direction == axis::X)
      disp = makePoint(straight, offset);
   else if (token.direction == -axis::X)
      disp = makePoint(-1, 0);
   else if (token.direction == axis::Y)
      disp = makePoint(offset, straight);
   else if (token.direction == -axis::Y)
      disp = makePoint(0, -1);
   else
      assert(false);

   return make_pair(origin + disp, token.direction);
}

void SBend::getEndpoints(vector<Point<int> >& output) const
{
   output.push_back(origin);

   if (axis == axis::X)
      output.push_back(origin + makePoint(straight - 1, offset));
   else
      output.push_back(origin + makePoint(offset, straight - 1));
}

void SBend::getCovers(vector<Point<int> >& output) const
{
   vector<Point<int> > exits;
   getEndpoints(exits);

   set<Point<int> > tmp;
   
   for (float f = 0.0f; f < 1.0f; f += 0.1f) {
      Vector<float> curveValue = curve(f);
      
      curveValue.z += 0.5f;
      
      int x, y;
      if (axis == axis::X) {
         x = static_cast<int>(floor(curveValue.x + origin.x));
         y = static_cast<int>(floor(curveValue.z + origin.y));
      }
      else {
         x = -static_cast<int>(floor(curveValue.z - origin.x));
         y = static_cast<int>(floor(curveValue.x + origin.y));
      }

      Point<int> p = makePoint(x, y);

      if (p != exits.at(0) && p != exits.at(1))
         tmp.insert(p);
   }

   copy(tmp.begin(), tmp.end(), back_inserter(output));
}

ITrackSegmentPtr SBend::mergeExit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

track::TravelToken SBend::getTravelToken(track::Position pos,
   track::Direction dir) const
{
   using namespace placeholders;
   
   ensureValidDirection(dir);

   track::TravelToken tok = {
      dir,
      pos,
      bind(&SBend::transform, this, _1, _2),
      track::flatGradientFunc,
      1
   };
   return tok;
}

void SBend::transform(const track::TravelToken& token, float delta) const
{
   assert(delta < curve.length);

   const bool backwards = token.direction == -axis;
   if (backwards)
      delta = curve.length - delta;

   const float curveDelta = delta / curve.length;

   Vector<float> curveValue = curve(curveDelta);

   const Vector<float> deriv = curve.deriv(curveDelta);
   const float angle =
      radToDeg<float>(atanf(deriv.z / deriv.x));

   float xTrans, yTrans;
   if (axis == axis::X) {
      xTrans = curveValue.x;
      yTrans = curveValue.z;
   }
   else if (axis == axis::Y) {
      xTrans = -curveValue.z;
      yTrans = curveValue.x;
   }
   else
      assert(false);

   glTranslatef(
      static_cast<float>(origin.x) + xTrans,
      height,
      static_cast<float>(origin.y) + yTrans);
      
   if (axis == axis::Y)
      glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
      
   glTranslatef(-0.5f, 0.0f, 0.0f);

   glRotatef(-angle, 0.0f, 1.0f, 0.0f);

   if (backwards)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
}

void SBend::ensureValidDirection(track::Direction dir) const
{
   if (!isValidDirection(dir))
      throw runtime_error
         ("Invalid direction on s-bend track: "
            + boost::lexical_cast<string>(dir)
            + " (should be parallel to "
            + boost::lexical_cast<string>(axis) + ")");
}

xml::element SBend::toXml() const
{
   return xml::element("sbendTrack")
      .addAttribute("align", axis == axis::X ? "x" : "y")
      .addAttribute("offset", offset)
      .addAttribute("straight", straight);
}

ITrackSegmentPtr makeSBend(track::Direction dir, int straight, int off)
{
   return ITrackSegmentPtr(new SBend(dir, straight, off));
}
