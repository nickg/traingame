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

#ifndef INC_ICONTROLLER_HPP
#define INC_ICONTROLLER_HPP

#include <memory>

// Actions the user can send
enum Action {
   BRAKE_TOGGLE,
   SHOVEL_COAL,
   THROTTLE_UP,
   THROTTLE_DOWN,
};

// Interface to something that can be controlled by the user
struct IController {
   virtual ~IController() {}

   virtual void actOn(Action anAction) = 0;

   // Get current values for the display
   virtual int throttle() const = 0;
   virtual bool brakeOn() const = 0;
   virtual double pressure() const = 0;
   virtual double temp() const = 0;
};

typedef std::shared_ptr<IController> IControllerPtr;

#endif
