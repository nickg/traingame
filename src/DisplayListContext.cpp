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

#include "IGraphics.hpp"

#include <stdexcept>

using namespace std;

// Special graphics context for use in display lists
class DisplayListContext : public IGraphics {
public:
   void setAmbient(double r, double g, double b)
   {
      throw runtime_error("setAmbient called in display list");
   }
   
   void setDiffuse(double r, double g, double b)
   {
      throw runtime_error("setDiffuse called in display list");
   }
   
   void moveLight(double x, double y, double z)
   {
      throw runtime_error("moveLight called in display list");
   }

   bool cuboidInViewFrustum(double x, double y, double z,
                            double sizeX, double sizeY, double sizeZ)
   {
      return true;
   }
   
   bool cubeInViewFrustum(double x, double y, double z, double size)
   {
      return true;
   }
   
   bool pointInViewFrustum(double x, double y, double z)
   {
      return true;
   }
  
   void setCamera(const Vector<double>& aPos,
                  const Vector<double>& aRotation)
   {
      throw runtime_error("setCamera called in display list");
   }
};

IGraphicsPtr makeDisplayListContext()
{
   return IGraphicsPtr(new DisplayListContext);
}

