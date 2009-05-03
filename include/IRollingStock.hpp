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

#include <tr1/memory>

// Interface for various powered and unpowered parts of the train
struct IRollingStock {
   virtual ~IRollingStock() {}

   // Update speed, fuel, etc.
   virtual void update(int aDelta) = 0;
   
   // Display the object and any animiation
   virtual void render() const = 0;

   // Return the controller for this vehicle (if it has one)
   virtual IControllerPtr controller() = 0;

   // Return the speed of the vehicle
   virtual double speed() const = 0;
};

typedef std::tr1::shared_ptr<IRollingStock> IRollingStockPtr;

// Make various waggons and engines
IRollingStockPtr makeEngine();

#endif
