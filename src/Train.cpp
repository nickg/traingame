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

#include "ITrain.hpp"
#include "IRollingStock.hpp"
#include "ILogger.hpp"
#include "TrackCommon.hpp"
#include "ISmokeTrail.hpp"

#include <stdexcept>
#include <cassert>

#include <GL/gl.h>

using namespace std;

// Concrete implementation of trains
class Train : public ITrain {
public:
   Train(IMapPtr aMap);
   
   void render() const;
   void update(int aDelta);

   Vector<float> front() const;
   
   double speed() const { return myParts.front().vehicle->speed(); }
   IControllerPtr controller() { return myParts.front().vehicle->controller(); }
private:
   // The different parts of the train are on different track segments
   struct Part {
      explicit Part(IRollingStockPtr aVehicle)
         : vehicle(aVehicle), segmentDelta(0.0) {}
      
      IRollingStockPtr vehicle;

      // The length of a track segment can be found by calling segmentLength()
      // This delta value ranges from 0 to that length and indicates how far
      // along the segment the train is
      ITrackSegmentPtr segment;
      double segmentDelta;
      ITrackSegment::TransformFunc transformer;
      
      // Direction train part is travelling along the track
      Vector<int> direction;
   };
   list<Part> myParts;

   const Part& engine() const;
   Part& engine();
   void move(double aDistance);
   void addPart(IRollingStockPtr aVehicle);
   Vector<float> partPosition(const Part& aPart) const;
   void updateSmokePosition(int aDelta);
   
   IMapPtr myMap;
   ISmokeTrailPtr mySmokeTrail;

   // Move part of the train across a connection
   void enterSegment(Part& aPart, const track::Connection& aConnection);

   // Seperation between waggons
   static const double SEPARATION;
};

const double Train::SEPARATION(0.1);

Train::Train(IMapPtr aMap)
   : myMap(aMap)
{
   myParts.push_front(Part(makeEngine()));
   
   enterSegment(engine(), aMap->startLocation());

   // Bit of a hack to put the engine in the right place
   move(0.275);
   
   for (int i = 1; i <= 5; i++)
      addPart(makeWaggon());

   mySmokeTrail = makeSmokeTrail();
}

void Train::addPart(IRollingStockPtr aVehicle)
{
   Part part(makeWaggon());
   enterSegment(part, myMap->startLocation());
   
   // Push the rest of the train along some
   move(part.vehicle->length() + SEPARATION);
   
   myParts.push_back(part);
}

Train::Part& Train::engine()
{
   assert(myParts.size() > 0);
   return myParts.front();
}

const Train::Part& Train::engine() const
{
   assert(myParts.size() > 0);
   return myParts.front();
}

// Move the train along the line a bit
void Train::move(double aDistance)
{
   for (list<Part>::iterator it = myParts.begin();
        it != myParts.end(); ++it) {

      // Never move in units greater than 1.0
      double d = aDistance;
      const double step = 0.25;
      do {
         (*it).segmentDelta += min(step, d);
         
         const double segmentLength = (*it).segment->segmentLength();
         if ((*it).segmentDelta >= segmentLength) {
            // Moved onto a new piece of track
            const double over = (*it).segmentDelta - segmentLength;
            enterSegment(*it, (*it).segment->nextPosition((*it).direction));
            (*it).segmentDelta = over;
         }

         d -= step;
      } while (d > 0.0);
   }
}

void Train::updateSmokePosition(int aDelta)
{
   const Part& e = engine();
   glPushMatrix();
   glLoadIdentity();

   e.transformer(e.segmentDelta);

   const float smokeOffX = 1.0f;
   const float smokeOffY = 1.0f;
   glTranslatef(smokeOffX, smokeOffY, 0.0f);
                
   float matrix[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, matrix);

   glPopMatrix();

   mySmokeTrail->setPosition(matrix[12], matrix[13], matrix[14]);
   mySmokeTrail->update(aDelta);
}

void Train::update(int aDelta)
{
   for (list<Part>::iterator it = myParts.begin();
        it != myParts.end(); ++it)
      (*it).vehicle->update(aDelta);

   updateSmokePosition(aDelta);
   
   // How many metres does a tile correspond to?
   const double M_PER_UNIT = 5.0;
   
   const double deltaSeconds = static_cast<float>(aDelta) / 1000.0f;
   move(engine().vehicle->speed() * deltaSeconds / M_PER_UNIT);
}

// Called when the train enters a new segment
// Resets the delta and gets the length of the new segment
void Train::enterSegment(Part& aPart, const track::Connection& aConnection)
{
   Point<int> pos;
   tie(pos, aPart.direction) = aConnection;
   
   //debug() << "Train part entered segment at " << pos
   //        << " moving " << aPart.direction;

   if (!myMap->isValidTrack(pos))
      throw runtime_error("Train fell off end of track!");

   aPart.segmentDelta = 0.0;
   aPart.segment = myMap->trackAt(pos);
   aPart.transformer = aPart.segment->transformFunc(aPart.direction);
}

void Train::render() const
{
   for (list<Part>::const_iterator it = myParts.begin();
        it != myParts.end(); ++it) {
      glPushMatrix();
      
      (*it).transformer((*it).segmentDelta);
      glTranslatef(0.0f, track::RAIL_HEIGHT, 0.0f);
      (*it).vehicle->renderModel();
      
      glPopMatrix();

      (*it).vehicle->renderEffects();
   }

   mySmokeTrail->render();
}

Vector<float> Train::front() const
{
   return partPosition(engine());
}

// Calculate the position of any train part
Vector<float> Train::partPosition(const Part& aPart) const
{
   // Call the transformer to compute the world location
   glPushMatrix();
   glLoadIdentity();

   aPart.transformer(aPart.segmentDelta);

   float matrix[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
   
   glPopMatrix();

   return makeVector(matrix[12], matrix[13], matrix[14]);
}

// Make an empty train
ITrainPtr makeTrain(IMapPtr aMap)
{
   return ITrainPtr(new Train(aMap));
}


