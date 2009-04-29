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
   IModelPtr myModel;

   double mySpeed, myMass, myBoilerPressure, myFireTemp;
   double myFuelOnFire;
   bool isBrakeOn;
   
   static const double MODEL_SCALE;
};

const double Engine::MODEL_SCALE(0.4);

Engine::Engine()
   : mySpeed(0.0), myMass(1000.0), myBoilerPressure(1.0),
     myFireTemp(0.0), myFuelOnFire(0.0), isBrakeOn(true)
{
   myModel = loadModel("train.obj", MODEL_SCALE);
}

// Draw the engine, smoke, etc.
void Engine::render() const
{        
   myModel->render();
}

// Compute the next state of the engine
void Engine::update()
{
   const double stopSpeed = 0.001;
   
   // Maximum friction
   const double Fmax = (abs(mySpeed) < stopSpeed ? MU_S : MU_K) * myMass;

   // Maximum braking force
   const double Bmax = isBrakeOn ? 2000.0 : 0.0;

   // `forwards' force
   const double drivingForce = myBoilerPressure;

   // Net force
   double netForce;
   if (Fmax + Bmax > drivingForce && abs(mySpeed) < stopSpeed)
      netForce = 0.0;
   else
      netForce = drivingForce - Fmax - Bmax;

   const double accel = netForce / myMass;
   
   // Consume some fuel
   const double burnRate = 0.999;
   myFuelOnFire *= burnRate;

   // The fire temparature is simply a function of fuel
   // TODO: find a better function!
   myFireTemp = min(myFuelOnFire * 100.0, 1000.0);

   // Boiler pressure is roughly proportional to temparature
   myBoilerPressure = myFireTemp * 10.0;

   // Accelerate the train
   mySpeed += accel / 1000.0;

   // Apply drag: drag is roughly proportional to the square of
   // velocity at high speeds
   const double dragCoeff = 0.1;
   mySpeed -= dragCoeff * mySpeed * mySpeed;

   debug() << "Hold: " << (Fmax + Bmax)
           << ", go: " << drivingForce
           << ", temp=" << myFireTemp
           << ", pressure=" << myBoilerPressure
           << ", accel=" << accel
           << ", drag=" << (dragCoeff * mySpeed * mySpeed);
   
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
