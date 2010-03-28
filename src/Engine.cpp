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
   Engine(IResourcePtr aRes);

   // IRollingStock interface
   void render() const;
   void update(int delta, float gradient);
   
   double speed() const { return mySpeed; }
   IControllerPtr controller() { return shared_from_this(); }
   float length() const { return model->dimensions().x; }

   // IController interface
   void actOn(Action anAction);
   int throttle() const { return myThrottle; }
   bool brakeOn() const { return isBrakeOn; }
   bool reverseOn() const { return reverse; }
   double pressure() const { return myBoilerPressure; }
   double temp() const { return myFireTemp; }
   bool stopped() const { return haveStopped; }

   // IXMLCallback interface
   void text(const string& localName, const string& aString);
private:
   double tractiveEffort() const;
   double resistance() const;
   double brakeForce() const;
   double gravity(float gradient) const;
   
   IModelPtr model;

   double mySpeed, myMass, myBoilerPressure, myFireTemp;
   double statTractiveEffort;
   bool isBrakeOn;
   int myThrottle;     // Ratio measured in tenths
   bool reverse;
   bool haveStopped;

   // Boiler pressure lags behind temperature
   MovingAverage<double, 1000> myBoilerDelay;

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

Engine::Engine(IResourcePtr aRes)
   : mySpeed(0.0), myMass(29.0),
     myBoilerPressure(INIT_PRESSURE),
     myFireTemp(INIT_TEMP),
     statTractiveEffort(34.7),
     isBrakeOn(true), myThrottle(0),
     reverse(false),
     haveStopped(true),
     resource(aRes)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/engine.xsd");

   parser->parse(resource->xmlFileName(), *this);
}

// Callback for loading elements from the XML file
void Engine::text(const string& localName, const string& aString)
{
   if (localName == "model")
      model = loadModel(resource, aString, MODEL_SCALE);
}

// Draw the engine model
void Engine::render() const
{        
   model->render();
}

// Calculate the current tractive effort
double Engine::tractiveEffort() const
{
   const double dir = reverse ? -1.0 : 1.0;
   
   if (abs(mySpeed) < TRACTIVE_EFFORT_KNEE)
      return statTractiveEffort * dir;
   else
      return (statTractiveEffort * TRACTIVE_EFFORT_KNEE)
         / abs(mySpeed)
         * dir;
}

// Calculate the magnitude of the resistance on the train
double Engine::resistance() const
{
   const double sign = mySpeed < 0.0 ? -1.0 : 1.0;
   
   const double a = 4.0;
   const double b = 0.05;
   const double c = 0.006;

   const double absSpeed = abs(mySpeed);
   
   return sign * (a + b*absSpeed + c*absSpeed*absSpeed);
}

// Calculate the resistance due to gravity on a slope
double Engine::gravity(float gradient) const
{
   const double g = 9.78;
   return -g * gradient * myMass;
}

// Calculate the magnitude of the braking force
double Engine::brakeForce() const
{
   const double beta = 0.09;
   const double g = 9.78;

   // Brake always acts against direction of motion
   double dir;
   if (mySpeed < 0.0)
      dir = -1.0;
   else
      dir = 1.0;
   
   return myMass * g * beta * dir;
}

// Compute the next state of the engine
void Engine::update(int delta, float gradient)
{
   // Update the pressure of the boiler
   // The fire temperature is delayed and then used to increase it
   myBoilerDelay << myFireTemp;
   myBoilerPressure = myBoilerDelay.value();
   
   const double P = tractiveEffort();
   const double Q = resistance();
   const double B = isBrakeOn ? brakeForce() : 0.0;
   const double G = gravity(gradient);   

#if 0
   static float lastGradient = gradient;
   if (abs(gradient - lastGradient) > 0.01f) {
      error() << "too big gradient change "
              << lastGradient << " -> "
              << gradient;
   }
   lastGradient = gradient;
#endif
   
   // The applied tractive effort is controlled by the throttle
   const double netP = P * static_cast<double>(myThrottle) / 10.0;

   const double deltaSeconds = delta / 1000.0f;
   const double a = ((netP - Q - B + G) / myMass) * deltaSeconds;

   //   mySpeed = max(mySpeed + a, 0.0);
   if (abs(mySpeed) < STOP_SPEED && myThrottle == 0 && isBrakeOn) {
      mySpeed = 0.0;
      haveStopped = true;
   }
   else {
      mySpeed += a;
      haveStopped = false;
   }

#if 0
   debug() << "P=" << netP << ", Q=" << Q
           << ", B=" << B
           << ", G=" << G
           << ", a=" << a << ", v=" << mySpeed
           << " (delta=" << delta << " grad=" << gradient << ")";
#endif
}

// User interface to the engine
void Engine::actOn(Action anAction)
{
   switch (anAction) {
   case BRAKE_TOGGLE:
      isBrakeOn = !isBrakeOn;
      break;
   case SHOVEL_COAL:
      myFireTemp += 10.0;
      break;
   case THROTTLE_UP:
      myThrottle = min(myThrottle + 1, 10);
      break;
   case THROTTLE_DOWN:
      myThrottle = max(myThrottle - 1, 0);
      break;
   case TOGGLE_REVERSE:
      reverse = !reverse;
      break;
   default:
      break;
   }
}

namespace {
   Engine* loadEngineXml(IResourcePtr aRes)
   {
      log() << "Loading engine from " << aRes->xmlFileName();

      return new Engine(aRes);
   }
}

// Load an engine from a resource file
IRollingStockPtr loadEngine(const string& aResId)
{
   static ResourceCache<Engine> cache(loadEngineXml, "engines");
   return cache.loadCopy(aResId);
}
