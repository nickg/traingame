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

#ifndef INC_IROLLING_STOCK_HPP
#define INC_IROLLING_STOCK_HPP

#include "Platform.hpp"
#include "IController.hpp"
#include "Maths.hpp"
#include "ICargo.hpp"

// Interface for various powered and unpowered parts of the train
struct IRollingStock {
   virtual ~IRollingStock() {}

   // Update speed, fuel, etc.
   virtual void update(int delta, double gravity) = 0;
   
   // Display the model
   virtual void render() const = 0;

   // Return the controller for this vehicle (if it has one)
   virtual IControllerPtr controller() = 0;

   // Return the speed of the vehicle
   virtual double speed() const = 0;

   // Return the length of the vehicle in game units
   virtual float length() const = 0;

   // Return the mass of the vehicle in tons
   virtual double mass() const = 0;

   // The cargo currently carried, if any
   virtual ICargoPtr cargo() const = 0;
};

typedef shared_ptr<IRollingStock> IRollingStockPtr;

// Make various waggons and engines
IRollingStockPtr load_engine(const string& a_res_id);
IRollingStockPtr load_waggon(const string& a_res_id);

#endif
