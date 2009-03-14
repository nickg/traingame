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

#include "IMap.hpp"

#include <GL/gl.h>

class Map : public IMap {
public:
   Map(int aWidth, int aDepth);

   int width() const { return myWidth; }
   int depth() const { return myDepth; }
   double heightAt() const { return 0.0; }

   void render() const;
private:
   int myWidth, myDepth;
};

Map::Map(int aWidth, int aDepth)
   : myWidth(aWidth), myDepth(aDepth)
{
   
}

void Map::render() const
{
   const double SCALE = 0.5;

   glDisable(GL_TEXTURE);
   glColor3d(0.0, 0.6, 0.0);
   for (int x = 0; x < myWidth; x++) {
      for (int z = 0; z < myDepth; z++) {
         double xd = static_cast<double>(x) * SCALE;
         double zd = static_cast<double>(z) * SCALE;

         glBegin(GL_QUADS);
         glNormal3d(0, 1, 0);
         glVertex3d(xd, 0, zd);
         glNormal3d(0, 1, 0);
         glVertex3d(xd + SCALE, 0, zd);
         glNormal3d(0, 1, 0);
         glVertex3d(xd + SCALE, 0, zd + SCALE);
         glNormal3d(0, 1, 0);
         glVertex3d(xd, 0, zd + SCALE);
         glEnd();
      }
   }
}

IMapPtr makeEmptyMap(int aWidth, int aDepth)
{
   return IMapPtr(new Map(aWidth, aDepth));
}
