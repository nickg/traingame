//
//  Copyright (C) 2010  Nick Gasson
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

#ifndef INC_MESSAGEAREA_HPP
#define INC_MESSAGEAREA_HPP

#include "Platform.hpp"

#include <string>

// Place to display messages at the bottom of the screen
struct IMessageArea {
   virtual ~IMessageArea() {}

   virtual void update(int delta) = 0;
   virtual void render() const = 0;

   virtual void post(
     const string& mess,       // Text to display
     int priority = 50,        // 0 (lowest) to 100 (highest)
     int delay = 100 ) = 0;    // Delay in milliseconds before fade
      
};

typedef shared_ptr<IMessageArea> IMessageAreaPtr;

IMessageAreaPtr makeMessageArea();

#endif
