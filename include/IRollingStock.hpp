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

#ifndef INC_IROLLING_STOCK_HPP
#define INC_IROLLING_STOCK_HPP

#include "IController.hpp"
#include "Maths.hpp"

#include <memory>

// Interface for various powered and unpowered parts of the train
struct IRollingStock {
   virtual ~IRollingStock() {}

   // Update speed, fuel, etc.
   virtual void update(int aDelta, Vector<float> aPosition) = 0;
   
   // Display the base object
   // This should also display any animation that is attached to
   // the model
   // Animation that is not attached to the model should use
   // `renderEffects' below
   virtual void renderModel() const = 0;

   // Display any effects that should occur with absolute
   // co-ordinates (like smoke trails)
   virtual void renderEffects() const = 0;

   // Return the controller for this vehicle (if it has one)
   virtual IControllerPtr controller() = 0;

   // Return the speed of the vehicle
   virtual double speed() const = 0;

   // Return the length of the vehicle in game units
   virtual double length() const = 0;
};

typedef std::shared_ptr<IRollingStock> IRollingStockPtr;

// Make various waggons and engines
IRollingStockPtr makeEngine();
IRollingStockPtr makeWaggon();

#endif
