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

#include "IRollingStock.hpp"
#include "IModel.hpp"
#include "ILogger.hpp"
#include "MovingAverage.hpp"
#include "IXMLParser.hpp"
#include "ResourceCache.hpp"

#include <GL/gl.h>

//
//    READ THIS FIRST: physics model used by the steam engine
//
// Note: everything here uses SI units unless otherwise stated
//
// The "tractive effort" is a measure of the power of a steam engine
// at a given velocity: P(v). Note: the value usually quoted on the
// Wikipedia entry for trains is the /starting/ tractive effort
// (i.e. P(0) = Fmax)
//
// Tractive effort is at its maximum value and constant up to some
// speed TRACTIVE_EFFORT_KNEE after which it decreases as 1/x
// These values are really engine-dependant, but we're simplifying here.
//   P(v) = {  Fmax, if v < TRACTIVE_EFFORT_KNEE
//          {  (Fmax * TRACTIVE_EFFORT_KNEE) / v, otherwise
//
// Resistance on the train is a combination of friction, drag, and some
// other sources. This is usually approximated by a quadratic:
//   Q(v) = a + bv + cv^2
// Where v is velocity. The constants a, b, and c are usually determined
// experimentally (we'll just have to guess). Where Q(v) intersects the
// tractive effort curve P(v) determines the train's maximum speed.
//
// Run models.gnuplot to see an example of these curves. The functions
// in that file should match the code here!!
//
// Currently pressure varies between 0 and 1. <0.1 and >1.0 are bad, but
// currently don't correspond to real values
//

// Concrete implementation of a steam engine
class Engine : public IRollingStock,
               public IController,
               public IXMLCallback,
               public enable_shared_from_this<Engine> {
public:
   Engine(IResourcePtr a_res);

   // IRollingStock interface
   void render() const;
   void update(int delta, double gravity);
   
   double speed() const { return my_speed; }
   double mass() const { return my_mass; }
   IControllerPtr controller() { return shared_from_this(); }
   float length() const { return model->dimensions().x; }
   ICargoPtr cargo() const;
   
   // IController interface
   void act_on(Action an_action);
   int throttle() const { return my_throttle; }
   bool brake_on() const { return is_brake_on; }
   bool reverse_on() const { return reverse; }
   double pressure() const { return my_boiler_pressure; }
   double temp() const { return my_fire_temp; }
   bool stopped() const { return have_stopped; }

   // IXMLCallback interface
   void text(const string& local_name, const string& a_string);
private:
   double tractive_effort() const;
   double resistance() const;
   double brake_force() const;
   
   IModelPtr model;

   double my_speed, my_mass, my_boiler_pressure, my_fire_temp;
   double stat_tractive_effort;
   bool is_brake_on;
   int my_throttle;     // Ratio measured in tenths
   bool reverse;
   bool have_stopped;

   // Boiler pressure lags behind temperature
   MovingAverage<double, 1000> my_boiler_delay;

   IResourcePtr resource;
   
   static const float MODEL_SCALE;
   static const double TRACTIVE_EFFORT_KNEE;

   static const double INIT_PRESSURE, INIT_TEMP;

   static const double STOP_SPEED;
};

const float Engine::MODEL_SCALE(0.4f);
const double Engine::TRACTIVE_EFFORT_KNEE(10.0);
const double Engine::INIT_PRESSURE(0.2);
const double Engine::INIT_TEMP(50.0);
const double Engine::STOP_SPEED(0.01);

Engine::Engine(IResourcePtr a_res)
   : my_speed(0.0), my_mass(29.0),
     my_boiler_pressure(INIT_PRESSURE),
     my_fire_temp(INIT_TEMP),
     stat_tractive_effort(34.7),
     is_brake_on(true), my_throttle(0),
     reverse(false),
     have_stopped(true),
     resource(a_res)
{
   static IXMLParserPtr parser = make_xml_parser("schemas/engine.xsd");

   parser->parse(resource->xml_file_name(), *this);
}

// Callback for loading elements from the XML file
void Engine::text(const string& local_name, const string& a_string)
{
   if (local_name == "model") {
      model = load_model(resource, a_string, MODEL_SCALE);
      model->cache();
   }
}

// Draw the engine model
void Engine::render() const
{        
   model->render();
}

// Calculate the current tractive effort
double Engine::tractive_effort() const
{
   const double dir = reverse ? -1.0 : 1.0;
   
   if (abs(my_speed) < TRACTIVE_EFFORT_KNEE)
      return stat_tractive_effort * dir;
   else
      return (stat_tractive_effort * TRACTIVE_EFFORT_KNEE)
         / abs(my_speed)
         * dir;
}

// Calculate the magnitude of the resistance on the train
double Engine::resistance() const
{
   const double sign = my_speed < 0.0 ? -1.0 : 1.0;
   
   const double a = 4.0;
   const double b = 0.05;
   const double c = 0.006;

   const double abs_speed = abs(my_speed);
   
   return sign * (a + b*abs_speed + c*abs_speed*abs_speed);
}

// Calculate the magnitude of the braking force
double Engine::brake_force() const
{
   const double beta = 0.09;
   const double g = 9.78;

   // Brake always acts against direction of motion
   double dir;
   if (my_speed < 0.0)
      dir = -1.0;
   else
      dir = 1.0;

   if (abs(my_speed) < STOP_SPEED)
      return 0.0;
   else 
      return my_mass * g * beta * dir;
}

// Compute the next state of the engine
void Engine::update(int delta, double gravity)
{
   // Update the pressure of the boiler
   // The fire temperature is delayed and then used to increase it
   my_boiler_delay << my_fire_temp;
   my_boiler_pressure = my_boiler_delay.value();
   
   const double P = tractive_effort();
   const double Q = resistance();
   const double B = is_brake_on ? brake_force() : 0.0;
   const double G = gravity;   
   
   // The applied tractive effort is controlled by the throttle
   const double netP = P * static_cast<double>(my_throttle) / 10.0;

   const double delta_seconds = delta / 1000.0f;
   const double a = ((netP - Q - B + G) / my_mass) * delta_seconds;

   if (abs(my_speed) < STOP_SPEED && my_throttle == 0) {
      if (is_brake_on)
         my_speed = 0.0;
      have_stopped = true;
   }
   else
      have_stopped = false;
   
   my_speed += a;
     
#if 0
   debug() << "P=" << netP << ", Q=" << Q
           << ", B=" << B
           << ", G=" << G
           << ", a=" << a << ", v=" << my_speed
           << " (delta=" << delta << " grad=" << gradient << ")";
#endif
}

ICargoPtr Engine::cargo() const
{
   return ICargoPtr();
}

// User interface to the engine
void Engine::act_on(Action an_action)
{
   switch (an_action) {
   case BRAKE_TOGGLE:
      is_brake_on = !is_brake_on;
      break;
   case SHOVEL_COAL:
      my_fire_temp += 10.0;
      break;
   case THROTTLE_UP:
      my_throttle = min(my_throttle + 1, 10);
      break;
   case THROTTLE_DOWN:
      my_throttle = max(my_throttle - 1, 0);
      break;
   case TOGGLE_REVERSE:
      reverse = !reverse;
      break;
   default:
      break;
   }
}

namespace {
   Engine* load_engine_xml(IResourcePtr a_res)
   {
      log() << "Loading engine from " << a_res->xml_file_name();

      return new Engine(a_res);
   }
}

// Load an engine from a resource file
IRollingStockPtr load_engine(const string& a_res_id)
{
   static ResourceCache<Engine> cache(load_engine_xml, "engines");
   return cache.load_copy(a_res_id);
}
