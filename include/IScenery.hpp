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

#ifndef INC_ISCENERY_HPP
#define INC_ISCENERY_HPP

#include "Platform.hpp"
#include "IXMLSerialisable.hpp"

// Static scenery such as trees
struct IScenery : IXMLSerialisable {
   virtual ~IScenery() {}
   
   virtual void render() const = 0;
   virtual void setPosition(float x, float y, float z) = 0;

   // Don't need these: what about buildings???
   virtual void setAngle(float angle) = 0;
   virtual const string& resId() const = 0;
};

typedef shared_ptr<IScenery> ISceneryPtr;

class AttributeSet;

ISceneryPtr loadTree(const string& name);
ISceneryPtr loadTree(const AttributeSet& attrs);

#endif
