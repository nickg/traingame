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

#include "ITrain.hpp"
#include "IRollingStock.hpp"
#include "ILogger.hpp"
#include "TrackCommon.hpp"
#include "ISmokeTrail.hpp"

#include <stdexcept>
#include <cassert>
#include <queue>
#include <sstream>

#include <GL/gl.h>
#include <boost/operators.hpp>

// Concrete implementation of trains
class Train : public ITrain {
public:
   Train(IMapPtr a_map);

   // ITrain interface
   void render() const;
   void update(int a_delta);
   Vector<float> front() const;
   ITrackSegmentPtr track_segment() const;
   track::Direction direction() const;
   track::Position tile() const { return engine().travel_token.position; }
   double speed() const { return parts.front().vehicle->speed(); }
   IControllerPtr controller() { return parts.front().vehicle->controller(); }
   
private:
   // The different parts of the train are on different track segments
   struct Part : boost::equality_comparable<Part> {
      explicit Part(IRollingStockPtr a_vehicle)
         : vehicle(a_vehicle), segment_delta(0.0), movement_sign(1.0)
      {}
      
      IRollingStockPtr vehicle;

      // The length of a track segment can be found by calling
      // segment_length() This delta value ranges from 0 to that length and
      // indicates how far along the segment the train is
      ITrackSegmentPtr segment;
      float segment_delta;
      track::TravelToken travel_token;
      
      // Direction train part is travelling along the track
      Vector<int> direction;

      // Handles reversal mid-segment
      float movement_sign;

      bool operator==(const Part& other) const
      {
         return this == &other;
      }
   };
   list<Part> parts;

   const Part& engine() const;
   Part& engine();
   void move(double a_distance);
   void add_part(IRollingStockPtr a_vehicle);
   Vector<float> part_position(const Part& a_part) const;
   void update_smoke_position(int a_delta);
   void move_part(Part& part, double distance);

   static track::Connection reverse_token(const track::TravelToken& token);
   static void transform_to_part(const Part& p);
   
   IMapPtr map;
   ISmokeTrailPtr smoke_trail;
   
   Vector<float> velocity_vector;

   // Move part of the train across a connection
   void enter_segment(Part& a_part, const track::Connection& a_connection);
   
   // Seperation between waggons
   static const double SEPARATION;
};

const double Train::SEPARATION(0.15);

Train::Train(IMapPtr a_map)
   : map(a_map), velocity_vector(make_vector(0.0f, 0.0f, 0.0f))
{
   parts.push_front(Part(load_engine("saddle")));
   
   enter_segment(engine(), a_map->start());

   // Bit of a hack to put the engine in the right place
   move(0.275);

#if 1
   for (int i = 1; i <= 4; i++)
      add_part(load_waggon("coal_truck"));
#endif

   smoke_trail = make_smoke_trail();
}

void Train::add_part(IRollingStockPtr a_vehicle)
{
   Part part(a_vehicle);
   enter_segment(part, map->start());
   
   // Push the rest of the train along some
   move(part.vehicle->length() + SEPARATION);
   
   parts.push_back(part);
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

void Train::move_part(Part& part, double distance)
{
   // Never move in units greater than 1.0
   double d = abs(distance);
   double sign = (distance >= 0.0 ? 1.0 : -1.0) * part.movement_sign;
   const double step = 0.25;
   
   //debug() << "move d=" << distance << " s=" << sign
   //        << " ms=" << part.movement_sign;
   
   do {
      part.segment_delta += min(step, d) * sign;
      
      const double segment_length =
         part.segment->segment_length(part.travel_token);
      if (part.segment_delta >= segment_length) {
         // Moved onto a new piece of track
         const double over = part.segment_delta - segment_length;
         enter_segment(part, part.segment->next_position(part.travel_token));
         part.segment_delta = over;
      }
      else if (part.segment_delta < 0.0) {
         track::Connection prev = reverse_token(part.travel_token);
         enter_segment(part, prev);
         part.segment_delta *= -1.0;
         part.movement_sign *= -1.0;
      }
      
      d -= step;
   } while (d > 0.0);
}

// Move the train along the line a bit
void Train::move(double a_distance)
{
   using namespace placeholders;
   
   for (list<Part>::iterator it = parts.begin();
        it != parts.end(); ++it)
      move_part(*it, a_distance);
}

void Train::update_smoke_position(int a_delta)
{
   const Part& e = engine();
   glPushMatrix();
   glLoadIdentity();

   transform_to_part(e);

   const float smoke_offX = 0.63f;
   const float smoke_offY = 1.04f;
   glTranslatef(smoke_offX, smoke_offY, 0.0f);
                
   float matrix[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, matrix);

   glPopMatrix();

   smoke_trail->set_position(matrix[12], matrix[13], matrix[14]);
   smoke_trail->set_velocity(
      velocity_vector.x,
      velocity_vector.y,
      velocity_vector.z);
   smoke_trail->update(a_delta);

   // Make the rate at which new particles are created proportional
   // to the throttle of the controller
   const int throttle = e.vehicle->controller()->throttle();
   const int base_delay = 200;

   smoke_trail->set_delay(base_delay - (throttle * 15));
}

void Train::update(int delta)
{
   double gravity_sum = 0.0;
   for (list<Part>::iterator it = parts.begin();
        it != parts.end(); ++it) {
      float gradient = (*it).travel_token.gradient((*it).segment_delta);

      if ((*it).direction.x < 0 || (*it).direction.z < 0)
         gradient *= -1.0f;

      gradient *= (*it).movement_sign;

      const double g = 9.78;
      gravity_sum += -g * gradient * (*it).vehicle->mass();
   }

   for (list<Part>::iterator it = parts.begin();
        it != parts.end(); ++it)
      (*it).vehicle->update(delta, gravity_sum);

   update_smoke_position(delta);
   
   // How many metres does a tile correspond to?
   const double M_PER_UNIT = 5.0;

   const Vector<float> old_pos = part_position(engine());
   
   const double delta_seconds = static_cast<float>(delta) / 1000.0f;
   move(engine().vehicle->speed() * delta_seconds / M_PER_UNIT);

   velocity_vector = part_position(engine()) - old_pos;
}

// Called when the train enters a new segment
// Resets the delta and gets the length of the new segment
void Train::enter_segment(Part& a_part, const track::Connection& a_connection)
{
   Point<int> pos;
   tie(pos, a_part.direction) = a_connection;

#if 0
   debug() << "Train part entered segment at " << pos
           << " moving " << a_part.direction;
#endif

   if (!map->is_valid_track(pos))
      throw runtime_error("Train fell off end of track!");

   a_part.segment_delta = 0.0;
   a_part.segment = map->track_at(pos);
   a_part.travel_token = a_part.segment->get_travel_token(pos, a_part.direction);
}

void Train::transform_to_part(const Part& p)
{
   p.travel_token.transform(p.segment_delta);
   
   // If we're going backwards, flip the train around
   if (p.movement_sign < 0.0)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
}

void Train::render() const
{
   for (list<Part>::const_iterator it = parts.begin();
        it != parts.end(); ++it) {
      glPushMatrix();
      
      transform_to_part(*it);
      glTranslatef(0.0f, track::RAIL_HEIGHT, 0.0f);
      
      (*it).vehicle->render();
      
      glPopMatrix();
   }

   smoke_trail->render();
}

ITrackSegmentPtr Train::track_segment() const
{
   return engine().segment;
}

Vector<float> Train::front() const
{
   return part_position(engine());
}

track::Direction Train::direction() const
{
   return engine().direction;
}

// Calculate the position of any train part
Vector<float> Train::part_position(const Part& a_part) const
{
   // Call the transformer to compute the world location
   glPushMatrix();
   glLoadIdentity();
   
   a_part.travel_token.transform(a_part.segment_delta);

   float matrix[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
   
   glPopMatrix();

   return make_vector(matrix[12], matrix[13], matrix[14]);
}

// Compute a connection object that reverses the train's
// direction of travel
track::Connection Train::reverse_token(const track::TravelToken& token)
{
   track::Position pos = make_point(
      token.position.x - token.direction.x,
      token.position.y - token.direction.z);

   track::Direction dir = -token.direction;

   return make_pair(pos, dir);      
}

// Make an empty train
ITrainPtr make_train(IMapPtr a_map)
{
   return ITrainPtr(new Train(a_map));
}
