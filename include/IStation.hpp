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

#include <tr1/memory>

// The different types of cargo that may be carried
enum Cargo {
   COAL
};

// A station occupies one of more track segments and supplies and
// accepts a set of cargo
// The information about which track segments it actually occupies
// are stored in the map
struct IStation {
   virtual ~IStation() {}
};

typedef std::tr1::shared_ptr<IStation> IStationPtr;

IStationPtr makeStation();

#endif
