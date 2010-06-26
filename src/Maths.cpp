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

#include "Platform.hpp"
#include "Maths.hpp"

#include <GL/gl.h>

// A utility function for debugging normal calculation
void draw_normal(const Vector<float>& a_position,
                const Vector<float>& a_normal)
{
   glPushAttrib(GL_ENABLE_BIT);
   
   glDisable(GL_LIGHTING);
   glDisable(GL_BLEND);
   
   glPushAttrib(GL_CURRENT_BIT);
   glColor3d(1.0, 0.0, 0.0);
   
   Vector<float> norm_pos = a_position + a_normal;
   
   glBegin(GL_LINES);
   
   glVertex3d(a_position.x, a_position.y, a_position.z);
   glVertex3d(norm_pos.x, norm_pos.y, norm_pos.z);
   
   glEnd();
   
   glPopAttrib();
   glPopAttrib();
}

// A rough guess at the gradient at a point on a curve
float approx_gradient(function<float (float)> a_func, float x)
{
   const float delta = 0.01f;

   const float x1 = x - delta;
   const float x2 = x + delta;

   const float y1 = a_func(x1);
   const float y2 = a_func(x2);

   return (y2 - y1) / (x2 - x1);
}
