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

#include "TrackCommon.hpp"

#include <GL/gl.h>

// Draw a sleeper in the current maxtrix location
void renderSleeper()
{
   const double sleeperWidth = 0.1;
   const double sleeperDepth = 0.05;
   const double sleeperOff = sleeperWidth / 2.0;
   
   glPushMatrix();
   glColor3d(0.5, 0.3, 0.0);
   glBegin(GL_QUADS);

   // Top
   glNormal3d(0.0, 1.0, 0.0);  // Up
   glVertex3d(-sleeperOff, sleeperDepth, -0.5);
   glVertex3d(sleeperOff, sleeperDepth, -0.5);
   glVertex3d(sleeperOff, sleeperDepth, 0.5);
   glVertex3d(-sleeperOff, sleeperDepth, 0.5);

   // Side 1
   glNormal3d(1.0, 0.0, 0.0);  // +ve x
   glVertex3d(sleeperOff, sleeperDepth, -0.5);
   glVertex3d(sleeperOff, 0.0, -0.5);
   glVertex3d(-sleeperOff, 0.0, -0.5);
   glVertex3d(-sleeperOff, sleeperDepth, -0.5);

   // Side 2
   glNormal3d(-1.0, 0.0, 0.0);  // -ve x
   glVertex3d(sleeperOff, sleeperDepth, 0.5);
   glVertex3d(sleeperOff, 0.0, 0.5);
   glVertex3d(-sleeperOff, 0.0, 0.5);
   glVertex3d(-sleeperOff, sleeperDepth, 0.5);

   // Front
   glNormal3d(0.0, 0.0, 1.0);  // +ve z
   glVertex3d(sleeperOff, sleeperDepth, 0.5);
   glVertex3d(sleeperOff, 0.0, 0.5);
   glVertex3d(sleeperOff, 0.0, -0.5);
   glVertex3d(sleeperOff, sleeperDepth, -0.5);

   // Back
   glNormal3d(0.0, 0.0, -1.0);  // -ve z
   glVertex3d(-sleeperOff, sleeperDepth, 0.5);
   glVertex3d(-sleeperOff, 0.0, 0.5);
   glVertex3d(-sleeperOff, 0.0, -0.5);
   glVertex3d(-sleeperOff, sleeperDepth, -0.5);
   
   
   glEnd();  // glBegin(GL_QUADS)
   glPopMatrix();
}

