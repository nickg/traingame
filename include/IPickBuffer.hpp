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

#ifndef INC_IPICKBUFFER_HPP
#define INC_IPICKBUFFER_HPP

#include "IGraphics.hpp"

#include <tr1/memory>

// Provides access to a pick buffer for selecting objects
struct IPickBuffer {
   virtual ~IPickBuffer() {}

   virtual IGraphicsPtr beginPick(int x, int y) = 0;
   virtual unsigned endPick() = 0;
};

typedef std::tr1::shared_ptr<IPickBuffer> IPickBufferPtr;

#endif
