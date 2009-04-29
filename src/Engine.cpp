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

#include <GL/gl.h>

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
   
   double speed() const { return mySpeed; }
   IControllerPtr controller() { return shared_from_this(); }

   // IController interface
   void actOn(Action anAction);
private:
   IModelPtr myModel;

   double mySpeed;
   
   static const double MODEL_SCALE;
};

const double Engine::MODEL_SCALE(0.4);

Engine::Engine()
   : mySpeed(0.0)
{
   myModel = loadModel("train.obj", MODEL_SCALE);
}

// Draw the engine, smoke, etc.
void Engine::render() const
{        
   myModel->render();
}

// User interface to the engine
void Engine::actOn(Action anAction)
{

}

// Make a new engine
IRollingStockPtr makeEngine()
{
   return IRollingStockPtr(new Engine);
}
