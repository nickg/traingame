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

#ifndef INC_ITEXTURE_HPP
#define INC_ITEXTURE_HPP

#include <tr1/memory>

// Abstract interface to textures
class ITexture {
public:
   virtual void bind() const = 0;
   virtual int width() const = 0;
   virtual int height() const = 0;
};

typedef std::tr1::shared_ptr<ITexture> ITexturePtr;

#endif
