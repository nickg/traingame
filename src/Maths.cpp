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

#include "Maths.hpp"

#include <GL/gl.h>

// A utility function for debugging normal calculation
void drawNormal(const Vector<float>& aPosition,
                const Vector<float>& aNormal)
{
   glPushAttrib(GL_ENABLE_BIT);
   
   glDisable(GL_LIGHTING);
   glDisable(GL_BLEND);
   
   glPushAttrib(GL_CURRENT_BIT);
   glColor3d(1.0, 0.0, 0.0);
   
   Vector<float> normPos = aPosition + aNormal;
   
   glBegin(GL_LINES);
   
   glVertex3d(aPosition.x, aPosition.y, aPosition.z);
   glVertex3d(normPos.x, normPos.y, normPos.z);
   
   glEnd();
   
   glPopAttrib();
   glPopAttrib();
}
