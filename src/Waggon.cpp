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

#include <stdexcept>

using namespace std;

// All cargo waggons
class Waggon : public IRollingStock {
public:
   Waggon();
   ~Waggon() {}

   void update(int aDelta, Vector<float> aPosition);
   void renderModel() const;
   void renderEffects() const {}
   IControllerPtr controller();
   double speed() const { return 0.0; }
   double length() const { return myModel->dimensions().x; }
private:
   IModelPtr myModel;

   static const double MODEL_SCALE;
};

const double Waggon::MODEL_SCALE(0.4);

Waggon::Waggon()
{
   myModel = loadModel("coal_truck.obj", MODEL_SCALE);
}

void Waggon::update(int aDelta, Vector<float> aPosition)
{
   
}

void Waggon::renderModel() const
{
   myModel->render();
}

IControllerPtr Waggon::controller()
{
   throw runtime_error("Cannot control a waggon!");
}

IRollingStockPtr makeWaggon()
{
   return IRollingStockPtr(new Waggon);
}

