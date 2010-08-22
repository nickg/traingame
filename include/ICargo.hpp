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

#ifndef INC_ICARGO_HPP
#define INC_ICARGO_HPP

#include "Platform.hpp"

enum CargoType {
   COAL
};

// A quantity of cargo loaded on a truck or waiting at a station
struct ICargo {
   virtual ~ICargo() {}

   virtual CargoType type() const = 0;
};

typedef shared_ptr<ICargo> ICargoPtr;

#endif
