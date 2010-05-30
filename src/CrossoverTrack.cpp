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
#include "OpenGLHelper.hpp"

#include <stdexcept>
#include <cassert>

#include <boost/lexical_cast.hpp>

using namespace placeholders;
using namespace boost;

// A section of track that allows travelling along both axis
class CrossoverTrack : public ITrackSegment,
                       public enable_shared_from_this<CrossoverTrack>,
                       private StraightTrackHelper,
                       private SleeperHelper {
public:
   CrossoverTrack() : myX(0), myY(0), height(0.0f) {}
   ~CrossoverTrack() {}

   void setOrigin(int x, int y, float h);
   
   void render() const {}
   void merge(IMeshBufferPtr buf) const;
   
   float segmentLength(const track::TravelToken& aToken) const;
   bool isValidDirection(const track::Direction& aDirection) const;
   track::Connection nextPosition(const track::TravelToken& aToken) const;
   void getEndpoints(vector<Point<int> >& aList) const;
   void getCovers(vector<Point<int> >& output) const { }
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);
   track::TravelToken getTravelToken(track::Position aPosition,
      track::Direction aDirection) const;
   void nextState() {}
   void prevState() {}
   bool hasMultipleStates() const { return false; }
   void setStateRenderHint() {}

   // IXMLSerialisable interface
   xml::element toXml() const;
   
private:
   void transform(const track::TravelToken& aToken, float delta) const;
   
   int myX, myY;
   float height;
};

void CrossoverTrack::merge(IMeshBufferPtr buf) const
{
   // Render the y-going rails and sleepers
   {    
      Vector<float> off = makeVector(
         static_cast<float>(myX),
         height,
         static_cast<float>(myY));
      
      mergeStraightRail(buf, off, 0.0f);

      off += makeVector(0.0f, 0.0f, -0.4f);
            
      for (int i = 0; i < 4; i++) {
         mergeSleeper(buf, off, 90.0f);
         off += makeVector(0.0f, 0.0f, 0.25f);
      }
   }

   // Render the x-going rails and sleepers
   {
      Vector<float> off = makeVector(
         static_cast<float>(myX),
         height,
         static_cast<float>(myY));
      
      mergeStraightRail(buf, off, 90.0f);
      
      off += makeVector(-0.4f, 0.0f, 0.0f);
            
      for (int i = 0; i < 4; i++) {
         mergeSleeper(buf, off, 0.0f);
         off += makeVector(0.25f, 0.0f, 0.0f);
      }
   }
}

void CrossoverTrack::setOrigin(int x, int y, float h )
{
   myX = x;
   myY = y;
   height = h;
}

float CrossoverTrack::segmentLength(const track::TravelToken& aToken) const
{
   return 1.0f;
}

track::TravelToken
CrossoverTrack::getTravelToken(track::Position aPosition,
   track::Direction aDirection) const
{
   if (!isValidDirection(aDirection))
      throw runtime_error
         ("Invalid direction on crossover: " + lexical_cast<string>(aDirection));

   track::TravelToken tok = {
      aDirection,
      aPosition,
      bind(&CrossoverTrack::transform, this, _1, _2),
      track::flatGradientFunc,
      1
   };
   return tok;
}

void CrossoverTrack::transform(const track::TravelToken& aToken,
   float delta) const
{
   assert(delta < 1.0);
   
   bool backwards = aToken.direction== -axis::X || aToken.direction == -axis::Y;

   if (backwards) {
      delta = 1.0f - delta;
   }

   track::Direction dir = backwards ? -aToken.direction : aToken.direction;

   const double xTrans = dir == axis::X ? delta : 0;
   const double yTrans = dir == axis::Y ? delta : 0;

   glTranslated(static_cast<double>(myX) + xTrans,
      height,
      static_cast<double>(myY) + yTrans);

   if (dir == axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (backwards)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

bool CrossoverTrack::isValidDirection(const track::Direction& aDirection) const
{
   return aDirection == axis::X || aDirection == axis::Y
      || aDirection == -axis::Y || aDirection == -axis::X;
}

track::Connection
CrossoverTrack::nextPosition(const track::TravelToken& aToken) const
{
   if (aToken.direction == axis::X)
      return make_pair(makePoint(myX + 1, myY), axis::X);
   else if (aToken.direction == -axis::X)
      return make_pair(makePoint(myX - 1, myY), -axis::X);
   else if (aToken.direction == axis::Y)
      return make_pair(makePoint(myX, myY + 1), axis::Y);
   else if (aToken.direction == -axis::Y)
      return make_pair(makePoint(myX, myY - 1), -axis::Y);
   else
      throw runtime_error
         ("Invalid direction on crossover: " + lexical_cast<string>(aToken.direction));
}

void CrossoverTrack::getEndpoints(vector<Point<int> >& aList) const
{
   aList.push_back(makePoint(myX, myY));
}

ITrackSegmentPtr CrossoverTrack::mergeExit(Point<int> where,
   track::Direction dir)
{
   if (where == makePoint(myX, myY)
      && isValidDirection(dir))
      return shared_from_this();

   // No way to extend a crossover
   return ITrackSegmentPtr();
}

xml::element CrossoverTrack::toXml() const
{
   return xml::element("crossoverTrack");
}

ITrackSegmentPtr makeCrossoverTrack()
{
   return ITrackSegmentPtr(new CrossoverTrack);
}
