//
//  Copyright (C) 2009-2010  Nick Gasson
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
#include "BezierCurve.hpp"
#include "Matrix.hpp"

#include <cassert>

#include <GL/gl.h>
#include <boost/lexical_cast.hpp>

// Forks in the track
class Points : public ITrackSegment,
               private StraightTrackHelper,
               private SleeperHelper,
               private BezierHelper {
public:
   Points(track::Direction aDirection, bool reflect);

   // ITrackSegment interface
   void render() const;
   void merge(IMeshBufferPtr buf) const;
   void setOrigin(int x, int y, float h) { myX = x; myY = y; height = h; }
   float segmentLength(const track::TravelToken& aToken) const;
   bool isValidDirection(const track::Direction& aDirection) const;
   track::Connection nextPosition(const track::TravelToken& aToken) const;
   void getEndpoints(vector<Point<int> >& aList) const;
   void getCovers(vector<Point<int> >& output) const;
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);
   track::TravelToken getTravelToken(track::Position aPosition,
      track::Direction aDirection) const;
   void nextState();
   void prevState();
   bool hasMultipleStates() const { return true; }
   void setStateRenderHint();

   // IXMLSerialisable interface
   xml::element toXml() const;
private:
   void transform(const track::TravelToken& aToken, float aDelta) const;
   void ensureValidDirection(track::Direction aDirection) const;
   void renderArrow() const;

   Point<int> displacedEndpoint() const;
   Point<int> straightEndpoint() const;

   enum State { TAKEN, NOT_TAKEN };
   
   int myX, myY;
   track::Direction myAxis;
   bool reflected;
   State state;
   float height;

   // Draw the arrow over the points if true
   mutable bool stateRenderHint;

   static const BezierCurve<float> myCurve, myReflectedCurve;
};

const BezierCurve<float> Points::myCurve = makeBezierCurve
   (makeVector(0.0f, 0.0f, 0.0f),
      makeVector(1.0f, 0.0f, 0.0f),
      makeVector(2.0f, 0.0f, 1.0f),
      makeVector(3.0f, 0.0f, 1.0f));

const BezierCurve<float> Points::myReflectedCurve = makeBezierCurve
   (makeVector(0.0f, 0.0f, 0.0f),
      makeVector(1.0f, 0.0f, 0.0f),
      makeVector(2.0f, 0.0f, -1.0f),
      makeVector(3.0f, 0.0f, -1.0f));
      
Points::Points(track::Direction aDirection, bool reflect)
   : myX(0), myY(0),
     myAxis(aDirection), reflected(reflect),
     state(NOT_TAKEN),
     height(0.0f),
     stateRenderHint(false)
{
   
}

void Points::setStateRenderHint() 
{
   stateRenderHint = true;
}

void Points::renderArrow() const
{
   glPushMatrix();
   glPushAttrib(GL_ENABLE_BIT);

   glEnable(GL_BLEND);

   glTranslatef(-0.5f, 0.11f, 0.0f);
   glColor4f(0.2f, 0.1f, 0.9f, 0.7f);

   const float headWidth = 0.25f;
    
   if (state == TAKEN) {
	
      const BezierCurve<float>& curve =
         reflected ? myReflectedCurve : myCurve;

      const float step = 0.1f;
      const float arrowLen = 0.7f;

      glDisable(GL_CULL_FACE);

      for (float t = 0.0f; t < arrowLen; t += step) {

         const Vector<float> v1 = curve(t);
         const Vector<float> v2 = curve(t + step);

         if (t >= arrowLen - step) {
            // Arrow head
            glBegin(GL_TRIANGLES);
            {
               glVertex3f(v1.x, 0.0f, v1.z - headWidth);
               glVertex3f(v2.x, 0.0f, v2.z);
               glVertex3f(v1.x, 0.0f, v1.z + headWidth);
            }
            glEnd();
         }
         else {
            glBegin(GL_QUADS);
            {
               glVertex3f(v1.x, 0.0f, v1.z - 0.1f);
               glVertex3f(v1.x, 0.0f, v1.z + 0.1f);
               glVertex3f(v2.x, 0.0f, v2.z + 0.1f);
               glVertex3f(v2.x, 0.0f, v2.z - 0.1f);
            }
            glEnd();
         }
      }
   }
   else {
      const float headLength = 0.3f;
	
      glBegin(GL_QUADS);
      {
         glVertex3f(0.0f, 0.0f, 0.1f);
         glVertex3f(2.0f - headLength, 0.0f, 0.1f);
         glVertex3f(2.0f - headLength, 0.0f, -0.1f);
         glVertex3f(0.0f, 0.0f, -0.1f);
      }
      glEnd();
	
      // Draw the arrow head
      glBegin(GL_TRIANGLES);
      {
         glVertex3f(2.0f - headLength, 0.0f, headWidth);
         glVertex3f(2.0f, 0.0f, 0.0f);
         glVertex3f(2.0f - headLength, 0.0f, -headWidth);
      }
      glEnd();
   }

   glPopAttrib();
   glPopMatrix();
}

void Points::merge(IMeshBufferPtr buf) const
{
   static IMeshBufferPtr railBuf = makeBezierRailMesh(myCurve);
   static IMeshBufferPtr reflectBuf = makeBezierRailMesh(myReflectedCurve);
   
   Vector<float> off = makeVector(
      static_cast<float>(myX),
      height,
      static_cast<float>(myY));
   
   float yAngle = 0.0f;
      
   if (myAxis == -axis::X)
      yAngle = 180.0f;
   else if (myAxis == -axis::Y)
      yAngle = 90.0f;
   else if (myAxis == axis::Y)
      yAngle = 270.0f;

   // Render the rails
   
   buf->merge(reflected ? reflectBuf : railBuf,
      off + rotateY(makeVector(-0.5f, 0.0f, 0.0f), yAngle),
      yAngle);
   
   {
      Vector<float> t = off;
      
      for (int i = 0; i < 3; i++) {
         const float a = yAngle + 90.0f;
         mergeStraightRail(buf, t, a);
         
         t += rotateY(makeVector(0.0f, 0.0f, 1.0f), a);
      }
   }

   // Draw the curved sleepers
   for (float i = 0.25f; i < 1.0f; i += 0.08f) {
      Vector<float> v = (reflected ? myReflectedCurve : myCurve)(i);

      Vector<float> t = makeVector(v.x - 0.5f, 0.0f, v.z);
      Vector<float> soff = off + rotateY(t, yAngle);
      const Vector<float> deriv =
         (reflected ? myReflectedCurve : myCurve).deriv(i);
      const float angle =
         radToDeg<float>(atanf(deriv.z / deriv.x));

      mergeSleeper(buf, soff, yAngle - angle);
   }
   
   // Draw the straight sleepers
   off -= rotateY(makeVector(0.4f, 0.0f, 0.0f), yAngle);
   
   for (int i = 0; i < 12; i++) {
      mergeSleeper(buf, off, yAngle);
      off += rotateY(makeVector(0.25f, 0.0f, 0.0f), yAngle);
   }
}

void Points::render() const
{
   if (stateRenderHint) {
      glPushMatrix();
      
      glTranslatef(
         static_cast<float>(myX),
         height,
         static_cast<float>(myY));

      if (myAxis == -axis::X)
         glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
      else if (myAxis == -axis::Y)
         glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
      else if (myAxis == axis::Y)
         glRotatef(270.0f, 0.0f, 1.0f, 0.0f);
         
      renderArrow();
      stateRenderHint = false;
      
      glPopMatrix();
   }
}

float Points::segmentLength(const track::TravelToken& aToken) const
{
   if (aToken.position == displacedEndpoint())
      return myCurve.length;
   else
      return 3.0f;
}

track::TravelToken Points::getTravelToken(track::Position position,
   track::Direction direction) const
{
   using namespace placeholders;
   
   ensureValidDirection(direction);

   const int nExits = position.x == myX && position.y == myY ? 2 : 1;
    
   track::TravelToken tok = {
      direction,
      position,
      bind(&Points::transform, this, _1, _2),
      track::flatGradientFunc,
      nExits
   };
    
   return tok;
}

void Points::transform(const track::TravelToken& aToken, float delta) const
{
   const float len = segmentLength(aToken);
   
   assert(delta < len);

   if (myX == aToken.position.x && myY == aToken.position.y
      && state == NOT_TAKEN) {

      if (aToken.direction == myAxis
         && (myAxis == -axis::X || myAxis == -axis::Y))
         delta -= 1.0f;
      
      const float xTrans =
         myAxis == axis::X ? delta
         : (myAxis == -axis::X ? -delta : 0.0f);
      const float yTrans =
         myAxis == axis::Y ? delta
         : (myAxis == -axis::Y ? -delta : 0.0f);
      
      glTranslatef(static_cast<float>(myX) + xTrans,
         height,
         static_cast<float>(myY) + yTrans);
      
      if (myAxis == axis::Y || myAxis == -axis::Y)
         glRotated(-90.0, 0.0, 1.0, 0.0);
      
      glTranslated(-0.5, 0.0, 0.0);
   }
   else if (aToken.position == straightEndpoint()) {
      delta = 2.0f - delta;

      if (aToken.direction == -myAxis
         && (myAxis == axis::X || myAxis == axis::Y))
         delta += 1.0f;
      
      const float xTrans =
         myAxis == axis::X ? delta
         : (myAxis == -axis::X ? -delta : 0.0f);
      const float yTrans =
         myAxis == axis::Y ? delta
         : (myAxis == -axis::Y ? -delta : 0.0f);
      
      glTranslatef(static_cast<float>(myX) + xTrans,
         height,
         static_cast<float>(myY) + yTrans);
      
      if (myAxis == axis::Y || myAxis == -axis::Y)
         glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
      
      glTranslatef(-0.5f, 0.0f, 0.0f);
   }
   else if (aToken.position == displacedEndpoint() || state == TAKEN) {
      // Curving onto the straight section
      float xTrans, yTrans, rotate;

      // We have a slight problem in that the domain of the curve
      // function is [0,1] but the delta is in [0,len] so we have
      // to compress the delta into [0,1] here
      const float curveDelta = delta / len;

      bool backwards = aToken.position == displacedEndpoint();
      
      const float fValue = backwards ? 1.0f - curveDelta : curveDelta;
      const Vector<float> curveValue = myCurve(fValue);
      
      // Calculate the angle that the tangent to the curve at this
      // point makes to (one of) the axis at this point
      const Vector<float> deriv = myCurve.deriv(fValue);
      const float angle =
         radToDeg<float>(atanf(deriv.z / deriv.x));

      if (myAxis == -axis::X) {
         xTrans = 1.0f - curveValue.x;
         yTrans = reflected ? curveValue.z : -curveValue.z;
         rotate = reflected ? angle : -angle;
      }
      else if (myAxis == axis::X) {
         xTrans = curveValue.x;
         yTrans = reflected ? -curveValue.z : curveValue.z;
         rotate = reflected ? angle : -angle;
      }
      else if (myAxis == -axis::Y) {
         xTrans = reflected ? -curveValue.z : curveValue.z;
         yTrans = 1.0f - curveValue.x;
         rotate = reflected ? angle : -angle;
      }
      else if (myAxis == axis::Y) {
         xTrans = reflected ? curveValue.z: -curveValue.z;
         yTrans = curveValue.x;
         rotate = reflected ? angle : -angle;
      }
      else
         assert(false);

      glTranslatef(
         static_cast<float>(myX) + xTrans,
         height,
         static_cast<float>(myY) + yTrans);
      
      if (myAxis == axis::Y || myAxis == -axis::Y)
         glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
      
      glTranslatef(-0.5f, 0.0f, 0.0f);

      glRotatef(rotate, 0.0f, 1.0f, 0.0f);
   }
   else
      assert(false);
   
   if (aToken.direction == -axis::X || aToken.direction == -axis::Y)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

void Points::ensureValidDirection(track::Direction aDirection) const
{
   if (!isValidDirection(aDirection))
      throw runtime_error
         ("Invalid direction on points: "
            + boost::lexical_cast<string>(aDirection)
            + " (should be parallel to "
            + boost::lexical_cast<string>(myAxis) + ")");
}

bool Points::isValidDirection(const track::Direction& aDirection) const
{
   if (myAxis == axis::X || myAxis == -axis::X)
      return aDirection == axis::X || -aDirection == axis::X;
   else
      return aDirection == axis::Y || -aDirection == axis::Y;
}

track::Connection Points::nextPosition(const track::TravelToken& aToken) const
{
   const bool branching = state == TAKEN;
         
   if (myAxis == axis::X) {
      if (aToken.direction == -axis::X) {
         // Two possible entry points
         return make_pair(makePoint(myX - 1, myY), -axis::X);
      }
      else {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(makePoint(myX + 3, myY - 1), axis::X);
            else
               return make_pair(makePoint(myX + 3, myY + 1), axis::X);
         }
         else
            return make_pair(makePoint(myX + 3, myY), axis::X);
      }
   }
   else if (myAxis == -axis::X) {
      if (aToken.direction == -axis::X) {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(makePoint(myX - 3, myY + 1), -axis::X);
            else
               return make_pair(makePoint(myX - 3, myY - 1), -axis::X);
         }
         else
            return make_pair(makePoint(myX - 3, myY), -axis::X);
      }
      else {
         // Two possible entry points
         return make_pair(makePoint(myX + 1, myY), axis::X);
      }
   }
   else if (myAxis == axis::Y) {
      if (aToken.direction == -axis::Y) {
         // Two possible entry points
         return make_pair(makePoint(myX, myY - 1), -axis::Y);
      }
      else {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(makePoint(myX + 1, myY + 3), axis::Y);
            else
               return make_pair(makePoint(myX - 1, myY + 3), axis::Y);
         }
         else
            return make_pair(makePoint(myX, myY + 3), axis::Y);
      }
   }
   else if (myAxis == -axis::Y) {
      if (aToken.direction == -axis::Y) {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(makePoint(myX - 1, myY - 3), -axis::Y);
            else
               return make_pair(makePoint(myX + 1, myY - 3), -axis::Y);
         }
         else
            return make_pair(makePoint(myX, myY - 3), -axis::Y);
      }
      else {
         // Two possible entry points
         return make_pair(makePoint(myX, myY + 1), axis::Y);
      }
   }
   else
      assert(false);
}

// Get the endpoint that follows the curve
Point<int> Points::displacedEndpoint() const
{
   const int reflect = reflected ? -1 : 1;

   if (myAxis == axis::X)
      return makePoint(myX + 2, myY + 1*reflect);
   else if (myAxis == -axis::X)
      return makePoint(myX - 2, myY - 1*reflect);
   else if (myAxis == axis::Y)
      return makePoint(myX - 1*reflect, myY + 2);
   else if (myAxis == -axis::Y)
      return makePoint(myX + 1*reflect, myY - 2);
   else
      assert(false);
}

// Get the endpoint that follows the straight track
Point<int> Points::straightEndpoint() const
{
   if (myAxis == axis::X)
      return makePoint(myX + 2, myY);
   else if (myAxis == -axis::X)
      return makePoint(myX - 2, myY);
   else if (myAxis == axis::Y)
      return makePoint(myX, myY + 2);
   else if (myAxis == -axis::Y)
      return makePoint(myX, myY - 2);
   else
      assert(false);
}

void Points::getEndpoints(vector<Point<int> >& aList) const
{
   aList.push_back(makePoint(myX, myY));
   aList.push_back(straightEndpoint());
   aList.push_back(displacedEndpoint());
}

void Points::getCovers(vector<Point<int> >& output) const
{
   const int reflect = reflected ? -1 : 1;

   if (myAxis == axis::X) {
      output.push_back(makePoint(myX + 1, myY + 1*reflect));
      output.push_back(makePoint(myX + 1, myY));
   }
   else if (myAxis == -axis::X) {
      output.push_back(makePoint(myX - 1, myY - 1*reflect));
      output.push_back(makePoint(myX - 1, myY));
   }
   else if (myAxis == axis::Y) {
      output.push_back(makePoint(myX - 1*reflect, myY + 1));
      output.push_back(makePoint(myX, myY + 1));
   }
   else if (myAxis == -axis::Y) {
      output.push_back(makePoint(myX + 1*reflect, myY - 1));
      output.push_back(makePoint(myX, myY - 1));
   }
   else
      assert(false);
}

ITrackSegmentPtr Points::mergeExit(Point<int> where, track::Direction dir)
{
   // Cant merge with anything
   return ITrackSegmentPtr();
}

xml::element Points::toXml() const
{ 
   return xml::element("points")
      .addAttribute("align",
         myAxis == axis::X ? "x"
         : (myAxis == -axis::X ? "-x"
            : (myAxis == axis::Y ? "y"
               : (myAxis == -axis::Y ? "-y" : "?"))))
      .addAttribute("reflect", reflected);
}

void Points::nextState()
{
   state = reflected ? NOT_TAKEN : TAKEN;
}

void Points::prevState()
{
   state = reflected ? TAKEN : NOT_TAKEN;
}

ITrackSegmentPtr makePoints(track::Direction aDirection, bool reflect)
{
   return ITrackSegmentPtr(new Points(aDirection, reflect));
}
