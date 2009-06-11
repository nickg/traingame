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

#include <cassert>

#include <GL/gl.h>
#include <boost/lexical_cast.hpp>

// Forks in the track
class Points : public ITrackSegment {
public:
   Points(track::Direction aDirection, bool reflect);

   // ITrackSegment interface
   void render() const;
   void setOrigin(int x, int y) { myX = x; myY = y; }
   double segmentLength(const track::TravelToken& aToken) const;
   bool isValidDirection(const track::Direction& aDirection) const;
   track::Connection nextPosition(const track::TravelToken& aToken) const;
   void getEndpoints(std::list<Point<int> >& aList) const;
   ITrackSegmentPtr mergeExit(const Point<int>& aPoint,
                              const track::Direction& aDirection);
   xml::element toXml() const;
   track::TravelToken getTravelToken(track::Position aPosition,
                                     track::Direction aDirection) const;
private:
   void transform(const track::TravelToken& aToken, double aDelta) const;
   void ensureValidDirection(track::Direction aDirection) const;

   Point<int> displacedEndpoint() const;
   Point<int> straightEndpoint() const;
   
   int myX, myY;
   track::Direction myAxis;
   bool amReflected;
};

Points::Points(track::Direction aDirection, bool reflect)
   : myX(0), myY(0),
     myAxis(aDirection), amReflected(reflect)
{
   
}

void Points::render() const
{   
   glPushMatrix();

   if (myAxis == -axis::X)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
   else if (myAxis == -axis::Y)
      glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
   else if (myAxis == axis::Y)
      glRotatef(270.0f, 0.0f, 1.0f, 0.0f);

   if (amReflected)
      renderReflectedHypTanRail();
   else
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

double Points::segmentLength(const track::TravelToken& aToken) const
{
   return 3.0;
}

track::TravelToken Points::getTravelToken(track::Position aPosition,
                                          track::Direction aDirection) const
{
   using namespace placeholders;
   
   ensureValidDirection(aDirection);

   track::TravelToken tok = {
      aDirection,
      aPosition
   };
   tok.transformer = bind(&Points::transform, this, tok, _1);
   return tok;
}

void Points::transform(const track::TravelToken& aToken, double aDelta) const
{   
   assert(aDelta < 3.0);

   debug() << myAxis << " " << aToken.direction << " r=" << amReflected;

   if (myX == aToken.position.x && myY == aToken.position.y) {
      debug() << "Section 1";

      if (aToken.direction == myAxis
          && (myAxis == -axis::X || myAxis == -axis::Y)) {
         debug() << "transforming";
         aDelta -= 1.0;
      }
      
      const double xTrans =
         myAxis == axis::X ? aDelta
         : (myAxis == -axis::X ? -aDelta : 0.0);
      const double yTrans =
         myAxis == axis::Y ? aDelta
         : (myAxis == -axis::Y ? -aDelta : 0.0);
      
      glTranslatef(static_cast<double>(myX) + xTrans,
                   0.0,
                   static_cast<double>(myY) + yTrans);
      
      if (myAxis == axis::Y || myAxis == -axis::Y)
         glRotated(-90.0, 0.0, 1.0, 0.0);
   }
   else if (aToken.position == straightEndpoint()) {
      debug() << "Section 2";
      
      aDelta = 2.0 - aDelta;

      if (aToken.direction == -myAxis
          && (myAxis == axis::X || myAxis == axis::Y))
         aDelta += 1.0;
      
      const double xTrans =
         myAxis == axis::X ? aDelta
         : (myAxis == -axis::X ? -aDelta : 0.0);
      const double yTrans =
         myAxis == axis::Y ? aDelta
         : (myAxis == -axis::Y ? -aDelta : 0.0);
      
      glTranslatef(static_cast<double>(myX) + xTrans,
                   0.0,
                   static_cast<double>(myY) + yTrans);
      
      if (myAxis == axis::Y || myAxis == -axis::Y)
         glRotated(-90.0, 0.0, 1.0, 0.0);
   }
   else if (aToken.position == displacedEndpoint()) {
      // Curving onto the straight section
      debug() << "Section 3";

      float xTrans, yTrans;
      
      if (myAxis == -axis::X && aToken.direction == axis::X) {
         xTrans = aDelta - 2.0f;

         if (amReflected)
            yTrans = hypTanCurveFunc(3.0f - aDelta);
         else
            yTrans = -hypTanCurveFunc(3.0f - aDelta);
      }
      else if (myAxis == axis::X && aToken.direction == -axis::X) {
         xTrans = 3.0f - aDelta;
         
         if (amReflected)
            yTrans = -hypTanCurveFunc(3.0f - aDelta);
         else
            yTrans = hypTanCurveFunc(3.0f - aDelta);
      }
      if (myAxis == -axis::Y && aToken.direction == axis::Y) {
         if (amReflected)
            xTrans = -hypTanCurveFunc(3.0f - aDelta);
         else
            xTrans = hypTanCurveFunc(3.0f - aDelta);
         
         yTrans = aDelta - 2.0f;
      }
      else if (myAxis == axis::Y && aToken.direction == -axis::Y) {
         if (amReflected)
            xTrans = hypTanCurveFunc(3.0f - aDelta);
         else
            xTrans = -hypTanCurveFunc(3.0f - aDelta);
         
         yTrans = 3.0f - aDelta;
      }
      else
         assert(false);

      glTranslatef(myX + xTrans, 0.0f, myY + yTrans);
      
      if (myAxis == axis::Y || myAxis == -axis::Y)
         glRotated(-90.0, 0.0, 1.0, 0.0);
   }
   else
      assert(false);

   glTranslated(-0.5, 0.0, 0.0);
   
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
   if (myAxis == axis::X) {
      if (aToken.direction == -axis::X) {
         // Two possible entry points
         return make_pair(makePoint(myX - 1, myY), -axis::X);
      }
      else {
         // Two possible exits
         return make_pair(makePoint(myX + 3, myY), axis::X);
      }
   }
   else if (myAxis == -axis::X) {
      if (aToken.direction == -axis::X) {
         // Two possible entry points
         return make_pair(makePoint(myX - 3, myY), -axis::X);
      }
      else {
         // Two possible exits
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
         return make_pair(makePoint(myX, myY + 3), axis::Y);
      }
   }
   else if (myAxis == -axis::Y) {
      if (aToken.direction == -axis::Y) {
         // Two possible entry points
         return make_pair(makePoint(myX, myY - 3), -axis::Y);
      }
      else {
         // Two possible exits
         return make_pair(makePoint(myX, myY + 1), axis::Y);
      }
   }
   else
      assert(false);
}

// Get the endpoint that follows the curve
Point<int> Points::displacedEndpoint() const
{
   const int reflect = amReflected ? -1 : 1;

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

void Points::getEndpoints(std::list<Point<int> >& aList) const
{
   aList.push_back(makePoint(myX, myY));
   aList.push_back(straightEndpoint());
   aList.push_back(displacedEndpoint());
}

ITrackSegmentPtr Points::mergeExit(const Point<int>& aPoint,
                                   const track::Direction& aDirection)
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
      .addAttribute("reflect", amReflected);
}

ITrackSegmentPtr makePoints(track::Direction aDirection, bool reflect)
{
   return ITrackSegmentPtr(new Points(aDirection, reflect));
}
