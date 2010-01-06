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
#include <queue>

#include <GL/gl.h>
#include <boost/operators.hpp>

// Concrete implementation of trains
class Train : public ITrain {
public:
   Train(IMapPtr aMap);
   
   void render() const;
   void update(int aDelta);

   Vector<float> front() const;
   ITrackSegmentPtr trackSegment() const;
   track::Direction direction() const;
   track::Position tile() const { return engine().travelToken.position; }
   
   double speed() const { return parts.front().vehicle->speed(); }
   IControllerPtr controller() { return parts.front().vehicle->controller(); }
private:
   // The different parts of the train are on different track segments
   struct Part : boost::equality_comparable<Part> {
      explicit Part(IRollingStockPtr aVehicle, bool amDriving = false)
         : vehicle(aVehicle), segmentDelta(0.0), isLeading(amDriving),
           movementSign(1.0)
      {}
      
      IRollingStockPtr vehicle;

      // The length of a track segment can be found by calling
      // segmentLength() This delta value ranges from 0 to that length and
      // indicates how far along the segment the train is
      ITrackSegmentPtr segment;
      double segmentDelta;
      track::TravelToken travelToken;
      
      // Direction train part is travelling along the track
      Vector<int> direction;

      // Turns to take if this is not the engine
      queue<track::Choice> followQueue;

      // True if this is driving the train
      bool isLeading;

      // Handles reversal mid-segment
      double movementSign;

      bool operator==(const Part& other) const
      {
         return this == &other;
      }
   };
   list<Part> parts;

   const Part& engine() const;
   const Part& leading() const;
   Part& engine();
   void move(double aDistance);
   void addPart(IRollingStockPtr aVehicle);
   Vector<float> partPosition(const Part& aPart) const;
   void updateSmokePosition(int aDelta);
   void makeFollow(track::Choice aChoice);
   void flipLeader();

   static track::Connection reverseToken(const track::TravelToken& token);
   static void transformToPart(const Part& p);
   
   
   IMapPtr map;
   ISmokeTrailPtr smokeTrail;
   
   Vector<float> velocityVector;

   // Move part of the train across a connection
   void enterSegment(Part& aPart, const track::Connection& aConnection);
   
   // Seperation between waggons
   static const double SEPARATION;
};

const double Train::SEPARATION(0.1);

Train::Train(IMapPtr aMap)
   : map(aMap), velocityVector(makeVector(0.0f, 0.0f, 0.0f))
{
   parts.push_front(Part(loadEngine("pclass"), true));
   
   enterSegment(engine(), aMap->start());

   // Bit of a hack to put the engine in the right place
   move(0.275);
   
   for (int i = 1; i <= 5; i++)
      addPart(loadWaggon("coal_truck"));

   smokeTrail = makeSmokeTrail();
}

void Train::addPart(IRollingStockPtr aVehicle)
{
   Part part(aVehicle);
   enterSegment(part, map->start());
   
   // Push the rest of the train along some
   move(part.vehicle->length() + SEPARATION);
   
   parts.push_back(part);
}

// Return the part that is leading the train - i.e. the
// first to encounter points, etc.
const Train::Part& Train::leading() const
{
   Part const* p = NULL;

   for (list<Part>::const_iterator it = parts.begin();
        it != parts.end(); ++it) {
      if ((*it).isLeading) {
         if (p)
            assert(false && "> 1 leading part!");
         else
            p = &(*it);
      }
   }

   assert(p);
   return *p;
}

// Make everything that's not the engine follow its choice
void Train::makeFollow(track::Choice aChoice)
{
   const Part& leader = leading();
   for (list<Part>::iterator it = parts.begin();
        it != parts.end(); ++it) {
      if (*it != leader)
         (*it).followQueue.push(aChoice);
   }
}

Train::Part& Train::engine()
{
   assert(parts.size() > 0);
   return parts.front();
}

const Train::Part& Train::engine() const
{
   assert(parts.size() > 0);
   return parts.front();
}

// Change the leader part from the front to the back or vice
// versa
void Train::flipLeader()
{
   if (leading() == engine()) {
      // Make the last waggon the leader
      engine().isLeading = false;
      parts.back().isLeading = true;
   }
   else {
      // Make the engine the leader
      parts.back().isLeading = false;
      engine().isLeading = true;
   }
}

// Move the train along the line a bit
void Train::move(double aDistance)
{

   for (list<Part>::iterator it = parts.begin();
        it != parts.end(); ++it) {

      // Never move in units greater than 1.0
      double d = abs(aDistance);
      double sign = (aDistance >= 0.0 ? 1.0 : -1.0) * (*it).movementSign;
      const double step = 0.25;

      //debug() << "move d=" << aDistance << " s=" << sign
      //        << " ms=" << (*it).movementSign;

      do {
         (*it).segmentDelta += min(step, d) * sign;
         
         const double segmentLength =
            (*it).segment->segmentLength((*it).travelToken);
         if ((*it).segmentDelta >= segmentLength) {
            // Moved onto a new piece of track
            const double over = (*it).segmentDelta - segmentLength;
            enterSegment(*it, (*it).segment->nextPosition((*it).travelToken));
            (*it).segmentDelta = over;
         }
         else if ((*it).segmentDelta < 0.0) {
            track::Connection prev = reverseToken((*it).travelToken);
            enterSegment(*it, prev);
            (*it).segmentDelta *= -1.0;
            (*it).movementSign *= -1.0;

            if ((*it).isLeading)
               flipLeader();
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

   transformToPart(e);

   const float smokeOffX = 0.63f;
   const float smokeOffY = 1.04f;
   glTranslatef(smokeOffX, smokeOffY, 0.0f);
                
   float matrix[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, matrix);

   glPopMatrix();

   smokeTrail->setPosition(matrix[12], matrix[13], matrix[14]);
   smokeTrail->setVelocity(
      velocityVector.x,
      velocityVector.y,
      velocityVector.z);
   smokeTrail->update(aDelta);

   // Make the rate at which new particles are created proportional
   // to the throttle of the controller
   const int throttle = e.vehicle->controller()->throttle();
   const int baseDelay = 200;

   smokeTrail->setDelay(baseDelay - (throttle * 15));
}

void Train::update(int aDelta)
{
   for (list<Part>::iterator it = parts.begin();
        it != parts.end(); ++it)
      (*it).vehicle->update(aDelta);

   updateSmokePosition(aDelta);
   
   // How many metres does a tile correspond to?
   const double M_PER_UNIT = 5.0;

   const Vector<float> oldPos = partPosition(engine());
   
   const double deltaSeconds = static_cast<float>(aDelta) / 1000.0f;
   move(engine().vehicle->speed() * deltaSeconds / M_PER_UNIT);

   velocityVector = partPosition(engine()) - oldPos;
}

// Called when the train enters a new segment
// Resets the delta and gets the length of the new segment
void Train::enterSegment(Part& aPart, const track::Connection& aConnection)
{
   Point<int> pos;
   tie(pos, aPart.direction) = aConnection;
   
   //debug() << "Train part entered segment at " << pos
   //        << " moving " << aPart.direction;

   if (!map->isValidTrack(pos))
      throw runtime_error("Train fell off end of track!");

   aPart.segmentDelta = 0.0;
   aPart.segment = map->trackAt(pos);
   aPart.travelToken = aPart.segment->getTravelToken(pos, aPart.direction);

   if (aPart.travelToken.choices.size() > 1) {
      track::Choice choice;
      
      if (aPart == leading()) {
         // Need to make a choice: see what the controller has pre-set
         choice = engine().vehicle->controller()->consumeChoice();
         makeFollow(choice);
      }
      else {
         // We're following another part so look in the follow queue
         assert(!aPart.followQueue.empty());

         choice = aPart.followQueue.front();
         aPart.followQueue.pop();
      }
      
      aPart.travelToken.activeChoice = choice;
   }
}

void Train::transformToPart(const Part& p)
{
   p.travelToken.transform(p.segmentDelta);
   
   // If we're going backwards, flip the train around
   if (p.movementSign < 0.0)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
}

void Train::render() const
{
   for (list<Part>::const_iterator it = parts.begin();
        it != parts.end(); ++it) {
      glPushMatrix();
      
      transformToPart(*it);
      glTranslatef(0.0f, track::RAIL_HEIGHT, 0.0f);
      
      (*it).vehicle->render();
      
      glPopMatrix();
   }

   smokeTrail->render();
}

ITrackSegmentPtr Train::trackSegment() const
{
   return engine().segment;
}

Vector<float> Train::front() const
{
   return partPosition(engine());
}

track::Direction Train::direction() const
{
   return engine().direction;
}

// Calculate the position of any train part
Vector<float> Train::partPosition(const Part& aPart) const
{
   // Call the transformer to compute the world location
   glPushMatrix();
   glLoadIdentity();
   
   aPart.travelToken.transform(aPart.segmentDelta);

   float matrix[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
   
   glPopMatrix();

   return makeVector(matrix[12], matrix[13], matrix[14]);
}

// Compute a connection object that reverses the train's
// direction of travel
track::Connection Train::reverseToken(const track::TravelToken& token)
{
   track::Position pos = makePoint(
      token.position.x - token.direction.x,
      token.position.y - token.direction.z);

   track::Direction dir = -token.direction;

   return make_pair(pos, dir);      
}

// Make an empty train
ITrainPtr makeTrain(IMapPtr aMap)
{
   return ITrainPtr(new Train(aMap));
}
