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

using namespace std;
using namespace std::tr1;

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

// Concrete implementation of a steam engine
class Engine : public IRollingStock,
               public IController,
               public enable_shared_from_this<Engine> {
public:
   Engine();

   // IRollingStock interface
   void render() const;
   void update();
   
   double speed() const { return mySpeed; }
   IControllerPtr controller() { return shared_from_this(); }

   // IController interface
   void actOn(Action anAction);
private:
   double tractiveEffort() const;
   double resistance() const;
   
   IModelPtr myModel;

   double mySpeed, myMass, myBoilerPressure, myFireTemp;
   double myFuelOnFire;
   double myStatTractiveEffort;
   bool isBrakeOn;
   
   static const double MODEL_SCALE;
   static const double TRACTIVE_EFFORT_KNEE;
};

const double Engine::MODEL_SCALE(0.4);
const double Engine::TRACTIVE_EFFORT_KNEE(10.0);

Engine::Engine()
   : mySpeed(0.0), myMass(29.0), myBoilerPressure(1.0),
     myFireTemp(0.0), myFuelOnFire(0.0),
     myStatTractiveEffort(34.7),
     isBrakeOn(true)
{
   myModel = loadModel("train.obj", MODEL_SCALE);
}

// Draw the engine, smoke, etc.
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
   const double a = 2.0;
   const double b = 0.02;
   const double c = 0.0035;
   return a + b*mySpeed + c*mySpeed*mySpeed;
}

// Compute the next state of the engine
void Engine::update()
{
   const double P = tractiveEffort();
   const double Q = resistance();

   // A fudge factor to make the acceleration look realistic
   const double MASS_TWEAK = 10.0;
   
   const double a = (P - Q) / (myMass * MASS_TWEAK);

   mySpeed += a;
   
   debug() << "P=" << P << ", Q=" << Q
           << ", a=" << a << ", v=" << mySpeed;
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
      myFuelOnFire += 1.0;
      break;
   }
}

// Make a new engine
IRollingStockPtr makeEngine()
{
   return IRollingStockPtr(new Engine);
}
