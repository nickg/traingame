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

#ifndef INC_ISTATION_HPP
#define INC_ISTATION_HPP

#include "Platform.hpp"
#include "Maths.hpp"
#include "Colour.hpp"

#include <string>

// A station occupies one of more track segments and supplies and
// accepts a set of cargo
// The information about which track segments it actually occupies
// are stored in the map
struct IStation {
   virtual ~IStation() {}

   // Return or set the name of the station
   // This is only used for the user's benefit and does not identify
   // the station in any way
   virtual const string& name() const = 0;
   virtual void set_name(const string& a_name) = 0;

   // A station has an ID that uniquely identifies it
   // Setting is allowed to support the map loader
   virtual int id() const = 0;
   virtual void set_id(int an_id) = 0;

   // A station has a random colour that is used to identify it when
   // the highlight is drawn
   virtual Colour highlight_colour() const = 0;
   virtual bool highlight_visible() const = 0;
   virtual void set_highlight_visible(bool on_off) = 0;
};

typedef shared_ptr<IStation> IStationPtr;

IStationPtr make_station();

#endif
