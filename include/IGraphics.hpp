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

#ifndef INC_IGRAPHICS_HPP
#define INC_IGRAPHICS_HPP

#include "Platform.hpp"
#include "Maths.hpp"

// Interface to stateful graphics things (lights, cameras, etc.)
struct IGraphics {
   virtual ~IGraphics() {}

   // Camera
   virtual bool cuboidInViewFrustum(float x, float y, float z,
                                    float sizeX, float sizeY, float sizeZ) = 0;
   virtual bool cubeInViewFrustum(float x, float y, float z,
                                  float size) = 0;
   virtual bool pointInViewFrustum(float x, float y, float z) = 0;
   virtual void setCamera(const Vector<float>& aPos,
                          const Vector<float>& aRotation) = 0;
   virtual void lookAt(const Vector<float> anEyePoint,
                       const Vector<float> aTargetPoint) = 0;
};

typedef std::tr1::shared_ptr<IGraphics> IGraphicsPtr;

#endif
