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

#include "IRollingStock.hpp"
#include "IModel.hpp"
#include "ILogger.hpp"
#include "MovingAverage.hpp"

#include <GL/gl.h>

using namespace std;

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
               public std::enable_shared_from_this<Engine> {
public:
   Engine();

   // IRollingStock interface
   void render() const;
   void update(int aDelta);
   
   double speed() const { return mySpeed; }
   IControllerPtr controller() { return shared_from_this(); }
   double length() const { return myModel->dimensions().x; }

   // IController interface
   void actOn(Action anAction);
   int throttle() const { return myThrottle; }
   bool brakeOn() const { return isBrakeOn; }
   double pressure() const { return myBoilerPressure; }
   double temp() const { return myFireTemp; }
private:
   double tractiveEffort() const;
   double resistance() const;
   double brakeForce() const;
   
   IModelPtr myModel;

   double mySpeed, myMass, myBoilerPressure, myFireTemp;
   double myStatTractiveEffort;
   bool isBrakeOn;
   int myThrottle;     // Ratio measured in tenths

   // Boiler pressure lags behind temperature
   MovingAverage<double, 1000> myBoilerDelay;
   
   static const double MODEL_SCALE;
   static const double TRACTIVE_EFFORT_KNEE;

   static const double INIT_PRESSURE, INIT_TEMP;
};

const double Engine::MODEL_SCALE(0.4);
const double Engine::TRACTIVE_EFFORT_KNEE(10.0);
const double Engine::INIT_PRESSURE(0.2);
const double Engine::INIT_TEMP(50.0);

Engine::Engine()
   : mySpeed(0.0), myMass(29.0),
     myBoilerPressure(INIT_PRESSURE),
     myFireTemp(INIT_TEMP),
     myStatTractiveEffort(34.7),
     isBrakeOn(true), myThrottle(0)
{
   myModel = loadModel("pclass.obj", MODEL_SCALE);
}

// Draw the engine model
void Engine::render() const
{        
   myModel->render();
}

// Calculate the current tractive effort
double Engine::tractiveEffort() const
{
   if (mySpeed < TRACTIVE_EFFORT_KNEE)
      return myStatTractiveEffort;
   else
      return (myStatTractiveEffort * TRACTIVE_EFFORT_KNEE) / mySpeed;
}

// Calculate the magnitude of the resistance on the train
double Engine::resistance() const
{
   const double a = 4.0;
   const double b = 0.05;
   const double c = 0.006;
   return a + b*mySpeed + c*mySpeed*mySpeed;
}

// Calculate the magnitude of the braking force
double Engine::brakeForce() const
{
   const double beta = 0.09;
   const double g = 9.78;
   return myMass * g * beta;
}

// Compute the next state of the engine
void Engine::update(int aDelta)
{
   // Update the pressure of the boiler
   // The fire temperature is delayed and then used to increase it
   myBoilerDelay << myFireTemp;
   myBoilerPressure = myBoilerDelay.value();
   
   const double P = tractiveEffort();
   const double Q = resistance();
   const double B = isBrakeOn ? brakeForce() : 0.0;

   // The applied tractive effort is controlled by the throttle
   const double netP = P * static_cast<double>(myThrottle) / 10.0;

   const double deltaSeconds = aDelta / 1000.0f;
   const double a = ((netP - Q - B) / myMass) * deltaSeconds;

   mySpeed = max(mySpeed + a, 0.0);
   
   /*debug() << "P=" << netP << ", Q=" << Q
           << ", B=" << B
           << ", a=" << a << ", v=" << mySpeed
           << " (delta=" << aDelta << ")";*/
}

// User interface to the engine
void Engine::actOn(Action anAction)
{
   switch (anAction) {
   case BRAKE_TOGGLE:
      isBrakeOn = !isBrakeOn;
      debug() << "Brake is" << (isBrakeOn ? "" : " not") << " on";
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
   default:
      break;
   }
}

// Make a new engine
IRollingStockPtr makeEngine()
{
   return IRollingStockPtr(new Engine);
}
