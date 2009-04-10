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

// Concrete implementation of powered rolling stock
class Engine : public IRollingStock {
public:
   Engine();
   
   void render() const;
private:
   IModelPtr myModel;
   
   static const double MODEL_SCALE;
};

const double Engine::MODEL_SCALE(0.4);

Engine::Engine()
{
   myModel = loadModel("train.obj", MODEL_SCALE);
}

// Draw the engine, smoke, etc.
void Engine::render() const
{
   glPushMatrix();

   glTranslated(-0.5, 0.0, 0.0);
  
   Vector<double> dim = myModel->dimensions();
   //   glTranslated(-(dim.x/2.0), 0.0, 0.0);
      
   myModel->render();

   glPopMatrix();
}

// Make a new engine
IRollingStockPtr makeEngine()
{
   return IRollingStockPtr(new Engine);
}
