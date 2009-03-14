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

#include "Maths.hpp"

#include <tr1/memory>

// Interface to stateful graphics things (lights, cameras, etc.)
class IGraphics {
public:
   // Lights
   virtual void setAmbient(double r, double g, double b) = 0;
   virtual void setDiffuse(double r, double g, double b) = 0;
   virtual void moveLight(double x, double y, double z) = 0;

   // Camera
   virtual bool cuboidInViewFrustum(double x, double y, double z,
                                    double sizeX, double sizeY, double sizeZ) = 0;
   virtual bool cubeInViewFrustum(double x, double y, double z,
                                  double size) = 0;
   virtual bool pointInViewFrustum(double x, double y, double z) = 0;
   virtual void setCamera(const Vector<double>& aPos) = 0;   
};

typedef std::tr1::shared_ptr<IGraphics> IGraphicsPtr;

#endif
